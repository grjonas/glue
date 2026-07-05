#include "resolver.h"

Resolver resolver_init(Parser* parser, Stmt* stmts)
{
    // Parser cleanup
    arrfree(parser->errs);

    *parser = (Parser)
    {
        .state   = PARSER_STATE_FREED   ,
        .txt     = NULL                 ,
        .tokens  = NULL                 ,
        .start   = -1                   ,
        .end     = -1                   ,
        .current = -1                   ,
        .errs    = NULL                 ,
    };

    Resolver resolver = (Resolver)
    {
        .txt              = parser->txt   ,
        .tokens           = parser->tokens,
        .stmts            = stmts         ,
        .arena            = parser->arena ,
        .type_variable_id = 0             ,
        .decl_id          = 0             ,
        .loop_depth       = 0             ,
        .inside_function  = false         ,
        .context          = NULL          ,
        .declarations     = NULL          ,
        .identifiers      = NULL          ,
        .errs             = NULL          ,
    };
    return resolver;
}

void resolver_free(Resolver* resolver)
{
    free((char*)resolver->txt);
    arrfree(resolver->tokens);
    arena_free(&resolver->arena);

    arrfree(resolver->context     );
    arrfree(resolver->declarations);
    arrfree(resolver->identifiers );
    arrfree(resolver->errs        );

    *resolver = (Resolver)
    {
        .txt              = NULL           ,
        .tokens           = NULL           ,
        .stmts            = NULL           ,
        .arena            = resolver->arena,
        .type_variable_id = 0              ,
        .decl_id          = 0              ,
        .loop_depth       = 0              ,
        .inside_function  = false          ,
        .context          = NULL           ,
        .declarations     = NULL           ,
        .identifiers      = NULL           ,
        .errs             = NULL           ,
    };
}

bool resolver_resolve_stmt_block(Resolver* resolver)
{
    Stmt* curr_stmt  = resolver->stmts;

    int    size = 0   ;
    Stmt** body = NULL;
    Snapshot snapshot ;

    size = curr_stmt->stmt.block.size;
    body = curr_stmt->stmt.block.body;

    snapshot = resolver_get_context_snapshot(resolver);
    for (int i = 0; i < size; ++i)
    {
        Stmt* stmt = body[i];

        resolver->stmts = stmt     ;
        if (!resolver_resolve_stmt(resolver))
        {
            return false;
        }
        resolver->stmts = curr_stmt;
    }
    resolver_restore_context_snapshot(resolver, snapshot);

    return true;
}

bool resolver_resolve_stmt_let(Resolver* resolver)
{
    Stmt* curr_stmt  = resolver->stmts;

    char    * identifier = NULL;
    TypeExpr* type_expr  = NULL;
    Expr    * expr       = NULL;
    Decl    * decl       = NULL;
    Snapshot snapshot;

    identifier = curr_stmt->stmt.let.identifier;
    type_expr  = curr_stmt->stmt.let.type      ;
    expr       = curr_stmt->stmt.let.expr      ;

    decl = resolver_declare_variable(resolver, identifier);
    resolver_push_decl_to_context(resolver, decl);

    snapshot = resolver_get_context_snapshot(resolver);
    if (type_expr != NULL)
    {
        if (!resolver_resolve_type_expr(resolver, type_expr))
        {
            return false;
        }
    }

    curr_stmt->stmt.let.decl = decl;

    if (expr != NULL)
    {
        if(!resolver_resolve_expr(resolver, expr))
        {
            return false;
        }
    }

    resolver_restore_context_snapshot(resolver, snapshot);

    return true;
}

bool resolver_resolve_stmt_expr(Resolver* resolver)
{
    Stmt* curr_stmt  = resolver->stmts;

    Expr* expr = NULL;

    expr = curr_stmt->stmt.expr;

    if (!resolver_resolve_expr(resolver, expr))
    {
        return false;
    }

    return true;
}

bool resolver_resolve_stmt_if(Resolver* resolver)
{
    Stmt* curr_stmt  = resolver->stmts;

    Expr* expr = NULL;
    Stmt* stmt = NULL;
    Snapshot snapshot;

    expr = curr_stmt->stmt.iff.condition;
    stmt = curr_stmt->stmt.iff.body     ;

    if (expr != NULL)
    {
        if (!resolver_resolve_expr(resolver, expr))
        {
            return false;
        }
    }
    snapshot = resolver_get_context_snapshot(resolver);
    resolver->stmts = stmt     ;

    if (!resolver_resolve_stmt(resolver))
    {
        return false;
    }

    resolver_restore_context_snapshot(resolver, snapshot);
    resolver->stmts = curr_stmt;

    return true;
}

bool resolver_resolve_stmt_while(Resolver* resolver)
{
    Stmt* curr_stmt  = resolver->stmts;

    Expr* expr = NULL;
    Stmt* stmt = NULL;
    Snapshot snapshot;

    expr = curr_stmt->stmt.whilee.condition;
    stmt = curr_stmt->stmt.whilee.body     ;

    if (!resolver_resolve_expr(resolver, expr))
    {
        return false;
    }

    snapshot = resolver_get_context_snapshot(resolver);
    resolver->stmts = stmt     ;
    resolver->loop_depth++;

    if (!resolver_resolve_stmt(resolver))
    {
        return false;
    }

    resolver_restore_context_snapshot(resolver, snapshot);
    resolver->stmts = curr_stmt;
    resolver->loop_depth--;

    return true;
}

bool resolver_resolve_stmt_break(Resolver* resolver)
{
    Stmt* curr_stmt  = resolver->stmts;

    if (resolver->loop_depth <= 0)
    {
        resolver_throw_compiler_error(resolver, (CompileError)
        {
            .kind   = ERROR_ERROR      ,
            .line   = curr_stmt->line  ,
            .column = curr_stmt->column,
            .length = curr_stmt->line  ,
            .msg    = "Statement resolution: Breaking while not in loop.",
        });
        return false;
    }

    return true;
}

bool resolver_resolve_stmt_continue(Resolver* resolver)
{
    Stmt* curr_stmt  = resolver->stmts;

    if (resolver->loop_depth <= 0)
    {
        resolver_throw_compiler_error(resolver, (CompileError)
        {
            .kind   = ERROR_ERROR      ,
            .line   = curr_stmt->line  ,
            .column = curr_stmt->column,
            .length = curr_stmt->line  ,
            .msg    = "Statement resolution: Continuing while not in loop.",
        });
        return false;
    }

    return true;
}

bool resolver_resolve_stmt_fn_inner(Resolver* resolver, StmtFn fn)
{
    if (!resolver_resolve_type_expr(resolver, fn.return_type))
    {
        return false;
    }

    for (int i = 0; i < fn.argc; ++i)
    {
        StmtFnArg* arg = fn.argv[i];

        if (arg->type != NULL)
        {
            TypeExpr* te = arg->type;
            if (!resolver_resolve_type_expr(resolver, te))
            {
                return false;
            }
        }
    }

    return true;
}

bool resolver_resolve_stmt_fn(Resolver* resolver)
{
    Stmt* curr_stmt  = resolver->stmts;

    StmtFn fn;
    Stmt*  stmt = NULL;
    Decl*  decl = NULL;
    Snapshot snapshot;

    fn   = curr_stmt->stmt.fn;
    stmt = fn.body;

    // Does not return NULL
    decl = resolver_declare_variable(resolver, fn.identifier);
    fn.decl = decl;
    resolver_push_decl_to_context(resolver, decl);

    snapshot = resolver_get_context_snapshot(resolver);

    if (!resolver_resolve_stmt_fn_inner(resolver, fn))
    {
        return false;
    }

    bool inside_function = resolver->inside_function;
    resolver->stmts = stmt;
    resolver->inside_function = true;

    // Assigning types to argument.
    if (!resolver_resolve_stmt(resolver))
    {
        return false;
    }

    resolver->stmts = curr_stmt;
    resolver->inside_function = inside_function;

    resolver_restore_context_snapshot(resolver, snapshot);

    return true;
}

bool resolver_resolve_stmt_return(Resolver* resolver)
{
    Stmt* curr_stmt  = resolver->stmts;

    Expr* expr = NULL;

    assert(curr_stmt->kind == STMT_RETURN);

    // TODO: Somehow bind this to it's respective function declaration.
    if (!resolver->inside_function)
    {
        resolver_throw_compiler_error(resolver, (CompileError)
        {
            .kind   = ERROR_ERROR      ,
            .line   = curr_stmt->line  ,
            .column = curr_stmt->column,
            .length = curr_stmt->line  ,
            .msg    = "Statement resolution: Cannot return while not in function.",
        });
        return false;
    }

    expr = curr_stmt->stmt.returnn.expr;
    if (expr != NULL)
    {
        if (!resolver_resolve_expr(resolver, expr))
        {
            return false;
        }
    }

    return true;
}

// On error - stmts is set to NULL.
bool resolver_resolve_stmt(Resolver* resolver)
{
    Stmt* curr_stmt  = NULL;
    bool result = false; // Assume failure

    curr_stmt = resolver->stmts;
    if (curr_stmt == NULL)
    {
        fprintf(stderr, "[%s:%d] Variable resolution: Found NULL statement.\n", __FILE__, __LINE__);
        exit(1);
    }

    switch (curr_stmt->kind)
    {
        case STMT_BLOCK   : result = resolver_resolve_stmt_block   (resolver); break;
        case STMT_LET     : result = resolver_resolve_stmt_let     (resolver); break;
        case STMT_EXPR    : result = resolver_resolve_stmt_expr    (resolver); break;
        case STMT_IF      : result = resolver_resolve_stmt_if      (resolver); break;
        case STMT_WHILE   : result = resolver_resolve_stmt_while   (resolver); break;
        case STMT_BREAK   : result = resolver_resolve_stmt_break   (resolver); break;
        case STMT_CONTINUE: result = resolver_resolve_stmt_continue(resolver); break;
        case STMT_FN      : result = resolver_resolve_stmt_fn      (resolver); break;
        case STMT_RETURN  : result = resolver_resolve_stmt_return  (resolver); break;

        default:
            fprintf(stderr, "[%s:%d] Variable resolution: Found statement of unknown kind.\n", __FILE__, __LINE__);
            exit(1);
    }

    resolver->stmts = curr_stmt;

    return result;
}

// TODO: expand this later to functions, once we implement them.
bool resolver_resolve_expr_primary(Resolver* resolver, Expr* expr)
{
    char * identifier = NULL;
    Decl * decl       = NULL;
    ExprPrimaryStruct expr_struct;

    switch (expr->expr.primary.kind)
    {
        case EXPR_PRIMARY_IDENTIFIER:
            identifier = expr->expr.primary.primary.identifier;

            decl = resolver_get_decl_by_identifier(resolver, identifier);
            if (decl == NULL)
            {
                return false;
            }
            else if (decl->kind != DECL_LET)
            {
                resolver_throw_compiler_error(resolver, (CompileError)
                {
                    .kind   = ERROR_ERROR      ,
                    .line   = expr->line  ,
                    .column = expr->column,
                    .length = expr->length,
                    .msg    = "Type resolution: Could not resolve expression identifier to variable declaration.",
                });
                return false;
            }

            expr->expr.primary.kind = EXPR_PRIMARY_DECL;
            expr->expr.primary.primary.decl = decl     ;
            break;

        case EXPR_PRIMARY_STRUCT:
            expr_struct = expr->expr.primary.primary.structt;
            for (int i = 0; i < expr_struct.argc; ++i)
            {
                ExprPrimaryStructField f = *(expr_struct.argv[i]);
                if (!resolver_resolve_expr(resolver, f.value))
                {
                    return false;
                }
            }
            break;

        default:
    }

    return true;
}

// 1) If type != NULL, then we bind then the type of the expression is equal to type.
// 2) Set variable to the most recent instance of identifier.
// TODO: Once we begin implementing inference, update this code.
bool resolver_resolve_expr(Resolver* resolver, Expr* expr)
{
    assert(expr != NULL);

    bool result = false; // Assume failure

    switch (expr->kind)
    {
        case EXPR_PRIMARY:
            result = resolver_resolve_expr_primary(resolver, expr);
            if (!result)
            {
                return false;
            }
            break;

        case EXPR_UNARY:
            result = resolver_resolve_expr(resolver, expr->expr.unary.unary);
            if (!result)
            {
                return false;
            }
            break;

        case EXPR_BINARY:
            result = resolver_resolve_expr(resolver, expr->expr.binary.left);
            if (!result)
            {
                return false;
            }
            result = resolver_resolve_expr(resolver, expr->expr.binary.right);
            if (!result)
            {
                return false;
            }
            break;

        case EXPR_FN:
            result = resolver_resolve_expr(resolver, expr->expr.fn.caller);
            if (!result)
            {
                return false;
            }
            for (int i = 0; i < expr->expr.fn.argc; ++i)
            {
                Expr* e = expr->expr.fn.argv[i];
                result = resolver_resolve_expr(resolver, e);
                if (!result)
                {
                    return false;
                }
            }
            break;

        default:
            fprintf(stderr, "[%s:%d] Variable resolution: Found expression of unknown kind.\n", __FILE__, __LINE__);
            exit(1);
    }

    return true;
}

bool resolver_resolve_type_expr_identifier(Resolver* resolver, TypeExpr* type_expr)
{
    assert(type_expr != NULL);
    assert(type_expr->kind == TYPE_EXPR_IDENTIFIER);

    char* identifier = NULL;
    Decl* decl       = NULL;

    identifier = type_expr->type_expr.identifier.identifier;; 
    decl       = resolver_get_decl_by_identifier(resolver, identifier);
    if (decl != NULL)
    {
        if
        (
            decl_is_type_variable(*decl)
            || decl_is_alias(*decl)
            || (decl_is_new_type(*decl) && decl_get_new_type_parameter_num(*decl) == 0)
        )
        {
            type_expr->kind = TYPE_EXPR_VARIABLE;
            type_expr->type_expr.variable.decl = decl;
        }
        else
        {
            resolver_throw_compiler_error(resolver, (CompileError)
            {
                .kind   = ERROR_ERROR      ,
                .line   = type_expr->line  ,
                .column = type_expr->column,
                .length = type_expr->length,
                .msg    = "Type resolution: Type variable can only be a type variable, an alias or a new type with no parameters.",
            });
            return false;
        }
    }
    else
    {
        decl = resolver_declare_type_variable(resolver, identifier);
        resolver_push_decl_to_context(resolver, decl);

        type_expr->kind = TYPE_EXPR_VARIABLE;
        type_expr->type_expr.variable.decl = decl;
        type_expr->type_expr.variable.argv = NULL;
        type_expr->type_expr.variable.argc = 0   ;
    }
    return true;
}

bool resolver_resolve_type_expr_instance(Resolver* resolver, TypeExpr* type_expr)
{
    assert(type_expr != NULL);

    char     * caller = NULL;
    int        argc   = 0   ;
    TypeExpr** argv   = NULL;
    Decl     * decl   = NULL;

    caller = type_expr->type_expr.instance.caller;
    argc   = type_expr->type_expr.instance.argc  ;
    argv   = type_expr->type_expr.instance.argv  ;

    decl = resolver_get_decl_by_identifier(resolver, caller);
    if
        (
            decl != NULL
            && decl_is_new_type(*decl)
            && decl_get_new_type_parameter_num(*decl) == argc
        )
    {
        for (int i = 0; i < argc; ++i)
        {
            TypeExpr* te = argv[i];
            bool result = resolver_resolve_type_expr(resolver, te);
            if (!result)
            {
                return false;
            }
        }

        type_expr->kind = TYPE_EXPR_VARIABLE;
        type_expr->type_expr.variable.decl = decl;
        type_expr->type_expr.variable.argv = argv;
        type_expr->type_expr.variable.argc = argc;

        return true;
    }
    else
    {
        resolver_throw_compiler_error(resolver, (CompileError)
        {
            .kind   = ERROR_ERROR      ,
            .line   = type_expr->line  ,
            .column = type_expr->column,
            .length = type_expr->line  ,
            .msg    = "Type resolution: Could not find declaration of new type that also has the required number of parameters.",
        });
        return false;
    }
}

bool resolver_resolve_type_expr(Resolver* resolver, TypeExpr* type_expr)
{
    if (type_expr == NULL)
    {
        return true;
    }

    switch (type_expr->kind)
    {
        // I'm not really sure why this would happen..
        case TYPE_EXPR_VARIABLE  :
            fprintf(stderr, "[%s:%d] Type expression resolution: Found expression of kind variable before resolution.\n", __FILE__, __LINE__);
            exit(1);

        case TYPE_EXPR_IDENTIFIER:
            return resolver_resolve_type_expr_identifier(resolver, type_expr);

        case TYPE_EXPR_NIL       : return true;
        case TYPE_EXPR_BOOL      : return true;
        case TYPE_EXPR_INT       : return true;
        case TYPE_EXPR_REAL      : return true;
        case TYPE_EXPR_STRING    : return true;

        case TYPE_EXPR_LIST      :
            return resolver_resolve_type_expr(resolver, type_expr->type_expr.list.type);

        case TYPE_EXPR_STRUCT    :
            for (int i = 0; i < type_expr->type_expr.structt.argc; ++i)
            {
                TypeExprStructField* field = type_expr->type_expr.structt.argv[i];
                bool result = resolver_resolve_type_expr(resolver, field->value);
                if (!result)
                {
                    return false;
                }
            }
            return true;

        case TYPE_EXPR_FN        :
            bool result = resolver_resolve_type_expr(resolver, type_expr->type_expr.fn.left);
            if (!result)
            {
                return false;
            }
            return resolver_resolve_type_expr(resolver, type_expr->type_expr.fn.right);

        case TYPE_EXPR_INSTANCE  :
            return resolver_resolve_type_expr_instance(resolver, type_expr);
    }

    fprintf(stderr, "[%s:%d] Type expression resolution: Failed to recognise type expression kind.\n", __FILE__, __LINE__);
    exit(1);
}

Snapshot resolver_get_context_snapshot(Resolver* resolver)
{
    return (Snapshot)
    {
        .context_length   = arrlen(resolver->context) ,
        // .decl_id          = resolver->decl_id         ,
        // .type_variable_id = resolver->type_variable_id,
    };
}

void resolver_restore_context_snapshot(Resolver* resolver, Snapshot snapshot)
{
    Snapshot curr_snapshot  = (Snapshot)
    {
        .context_length   = arrlen(resolver->context) ,
        // .decl_id          = resolver->decl_id         ,
        // .type_variable_id = resolver->type_variable_id,
    };

    assert(curr_snapshot.context_length >= snapshot.context_length);
    // assert(curr_snapshot.decl_id >= snapshot.decl_id);
    // assert(curr_snapshot.type_variable_id >= snapshot.type_variable_id);

    for (int i = 0; i < curr_snapshot.context_length - snapshot.context_length; ++i)
    {
        (void) arrpop(resolver->context);
    }
    // resolver->decl_id = snapshot.decl_id;
    // resolver->type_variable_id = snapshot.type_variable_id;
}

void resolver_push_decl_to_context(Resolver* resolver, Decl* decl)
{
    arrput(resolver->context, decl);
}


char* resolver_get_existing_identifier(Resolver* resolver, char* identifier)
{
    int id_len = strlen(identifier);
    int len = arrlen(resolver->identifiers);
    char* new_id = NULL;

    for (int i = 0; i < len; ++i)
    {
        char* id = resolver->identifiers[i];
        if (strlen(id) == id_len && memcmp(id, identifier, id_len) == 0)
        {
            return id;
        }
    }

    new_id = (char*) arena_push_empty(&resolver->arena, (id_len + 1) * sizeof(char));
    arrput(resolver->identifiers, new_id);

    memcpy(new_id, identifier, id_len);

    return new_id;
}

void decl_set_type(Decl* decl, Type* type)
{
    decl->type = type;
}

Decl* resolver_get_decl_by_identifier(Resolver* resolver, char* identifier)
{
    int id_len = strlen(identifier);
    int len = arrlen(resolver->context);
    Decl* decl = NULL;
    char* id   = NULL;

    for (int i = len - 1; 0 <= i; --i)
    {
        decl = resolver->context[i];
        id   = NULL;
        switch (decl->kind)
        {
            case DECL_LET:
                id = decl->identifier;
                break;

            case DECL_TYPE_VARIABLE:
                id = decl->identifier;
                break;

            case DECL_ALIAS:
                id = decl->identifier;
                break;

            case DECL_TYPE:
                id = decl->identifier;
                break;

            case DECL_TYPE_CONSTRUCTOR:
                id = decl->identifier;
                break;

            default:
        }

        if (strlen(id) == id_len && memcmp(id, identifier, id_len) == 0)
        {
            return decl;
        }
    }

    return NULL;
}

Decl* resolver_declare_variable(Resolver* resolver, char* identifier)
{
    Decl decl;
    Decl* decl_ptr;
    char* existing_identifier = resolver_get_existing_identifier(resolver, identifier);

    decl = (Decl)
    {
        .kind       = DECL_LET           ,
        .identifier = existing_identifier,
        .type       = NULL               ,
        .id         = resolver->decl_id++,
    };

    decl_ptr = (Decl*) arena_push(&resolver->arena, &decl, sizeof(Decl));
    arrput(resolver->declarations, decl_ptr);

    return decl_ptr;
}

Decl* resolver_declare_type_variable(Resolver* resolver, char* identifier)
{
    Decl decl;
    Decl* decl_ptr;
    char* existing_identifier = resolver_get_existing_identifier(resolver, identifier);

    decl = (Decl)
    {
        .kind       = DECL_TYPE_VARIABLE ,
        .identifier = existing_identifier,
        .type       = NULL               ,
        .id         = resolver->decl_id++,
    };

    decl_ptr = (Decl*) arena_push(&resolver->arena, &decl, sizeof(Decl));
    arrput(resolver->declarations, decl_ptr);

    return decl_ptr;
}

void resolver_throw_compiler_error(Resolver* resolver, CompileError err)
{
    CompileError* err_ptr = NULL;
    err_ptr = (CompileError*) arena_push(&resolver->arena, &err, sizeof(CompileError));
    arrput(resolver->errs, err_ptr);
}
