#include "resolver.h"

Resolver resolver_init(Parser* parser, Stmt* stmts)
{
    // Parser cleanup
    arrfree(parser->errs);

    Resolver resolver = (Resolver)
    {
        .txt             = parser->txt   ,
        .tokens          = parser->tokens,
        .stmts           = stmts         ,
        .arena           = parser->arena ,
        .loop_depth      = 0             ,
        .inside_function = false         ,
        .context         = NULL          ,
        .declarations    = NULL          ,
        .identifiers     = NULL          ,
        .errs            = NULL          ,
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
        .txt             = NULL                    ,
        .tokens          = NULL                    ,
        .stmts           = NULL                    ,
        .arena           = resolver->arena         ,
        .loop_depth      = 0                       ,
        .inside_function = false                   ,
        .context         = NULL                    ,
        .declarations    = NULL                    ,
        .identifiers     = NULL                    ,
        .errs            = NULL                    ,
    };
}

// On error - stmts is set to NULL.
void resolver_resolve_stmt(Resolver* resolver)
{
    Stmt* curr_stmt  = NULL;

    char    * identifier = NULL;
    TypeExpr* type_expr  = NULL;
    Expr    * expr       = NULL;
    Stmt    * stmt       = NULL;
    int       size       = 0   ;
    Stmt   ** body       = NULL;
    StmtFn    fn               ;

    Type    * type       = NULL;
    Decl    * decl       = NULL;

    int snapshot = 0;

    curr_stmt = resolver->stmts;
    if (curr_stmt == NULL)
    {
        fprintf(stderr, "[%s:%d] Variable resolution: Found NULL statement.\n", __FILE__, __LINE__);
        exit(1);
    }

    switch (curr_stmt->kind)
    {
        case STMT_ERR     :
            fprintf(stderr, "[%s:%d] Variable resolution: Found statement of kind STMT_ERR.\n", __FILE__, __LINE__);
            exit(1);
            break;

        case STMT_BLOCK   :
            size = curr_stmt->stmt.block.size;
            body = curr_stmt->stmt.block.body;

            snapshot = resolver_get_context_snapshot(resolver);
            for (int i = 0; i < size; ++i)
            {
                Stmt* stmt = body[i];

                resolver->stmts = stmt     ;
                resolver_resolve_stmt(resolver);
                resolver->stmts = curr_stmt;
            }
            resolver_restore_context_snapshot(resolver, snapshot);
            break;

        case STMT_LET     :
            identifier = curr_stmt->stmt.let.identifier;
            type_expr  = curr_stmt->stmt.let.type      ;
            expr       = curr_stmt->stmt.let.expr      ;

            if (type_expr != NULL)
            {
                type = resolver_resolve_type_expr(resolver, type_expr);
                if (type == NULL)
                {
                    resolver->stmts = NULL;
                    return;
                }
            }
            decl = resolver_declare_let(resolver, identifier, type);
            resolver_push_decl_to_context(resolver, decl);
            curr_stmt->stmt.let.decl = decl;

            if (expr != NULL)
            {
                resolver_resolve_expr(resolver, &expr);
                if (expr == NULL)
                {
                    resolver->stmts = NULL;
                    return;
                }
            }
            break;

        case STMT_EXPR    :
            expr = curr_stmt->stmt.expr;

            resolver_resolve_expr(resolver, &expr);
            if (expr == NULL)
            {
                resolver->stmts = NULL;
                return;
            }
            break;

        case STMT_IF      :
            expr = curr_stmt->stmt.iff.condition;
            stmt = curr_stmt->stmt.iff.body     ;

            if (expr != NULL)
            {
                resolver_resolve_expr(resolver, &expr);
                if (expr == NULL)
                {
                    resolver->stmts = NULL;
                    return;
                }
            }
            snapshot = resolver_get_context_snapshot(resolver);
            resolver->stmts = stmt     ;

            resolver_resolve_stmt(resolver);

            resolver_restore_context_snapshot(resolver, snapshot);
            resolver->stmts = curr_stmt;
            break;

        case STMT_WHILE   :
            expr = curr_stmt->stmt.whilee.condition;
            stmt = curr_stmt->stmt.whilee.body     ;

            resolver_resolve_expr(resolver, &expr);
            if (expr == NULL)
            {
                resolver->stmts = NULL;
                return;
            }

            snapshot = resolver_get_context_snapshot(resolver);
            resolver->stmts = stmt     ;
            resolver->loop_depth++;

            resolver_resolve_stmt(resolver);

            resolver_restore_context_snapshot(resolver, snapshot);
            resolver->stmts = curr_stmt;
            resolver->loop_depth--;
            break;

        case STMT_BREAK   :
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
                resolver->stmts = NULL;
                return;
            }
            break;

        case STMT_CONTINUE:
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
                resolver->stmts = NULL;
                return;
            }
            break;
        case STMT_FN:
            fn   = curr_stmt->stmt.fn;
            stmt = fn.body;

            // Does not return NULL
            type = resolver_resolve_stmt_fn_type(resolver, fn);

            decl = resolver_declare_let(resolver, fn.identifier, type);
            resolver_push_decl_to_context(resolver, decl);

            bool inside_function = resolver->inside_function;

            snapshot = resolver_get_context_snapshot(resolver);
            resolver->stmts = stmt;
            resolver->inside_function = true;

            // Assigning types to argument.
            resolver_declare_fn_params(resolver, fn, type);

            resolver_resolve_stmt(resolver);

            resolver_restore_context_snapshot(resolver, snapshot);
            resolver->stmts = curr_stmt;
            resolver->inside_function = inside_function;
            break;

        case STMT_RETURN  :
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
                resolver->stmts = NULL;
                return;
            }

            expr = curr_stmt->stmt.returnn.expr;
            if (expr != NULL)
            {
                resolver_resolve_expr(resolver, &expr);
                if (expr == NULL)
                {
                    resolver->stmts = NULL;
                    return;
                }
            }
            break;

        default:
            fprintf(stderr, "[%s:%d] Variable resolution: Found statement of unknown kind.\n", __FILE__, __LINE__);
            exit(1);
    }

    // return curr_stmt;
    resolver->stmts = curr_stmt;
}

// 1) If type != NULL, then we bind then the type of the expression is equal to type.
// 2) Set variable to the most recent instance of identifier.
// TODO: Once we begin implementing inference, update this code.
void resolver_resolve_expr(Resolver* resolver, Expr** expr)
{
    assert(expr  != NULL);
    assert(*expr != NULL);

    char* identifier = NULL;
    Decl* decl       = NULL;
    ExprPrimaryStruct expr_struct;

    switch ((*expr)->kind)
    {
        case EXPR_PRIMARY:
            // TODO: expand this later to functions, once we implement them.
            switch ((*expr)->expr.primary.kind)
            {
                case EXPR_PRIMARY_IDENTIFIER:
                    identifier = (*expr)->expr.primary.primary.identifier;

                    decl = resolver_get_decl_by_identifier(resolver, identifier);
                    if (decl == NULL)
                    {
                        expr = NULL;
                        return;
                    }
                    else if (decl->kind != DECL_LET)
                    {
                        resolver_throw_compiler_error(resolver, (CompileError)
                        {
                            .kind   = ERROR_ERROR      ,
                            .line   = (*expr)->line  ,
                            .column = (*expr)->column,
                            .length = (*expr)->length,
                            .msg    = "Type resolution: Could not resolve expression identifier to variable declaration.",
                        });
                        expr = NULL;
                        return;
                    }

                    (*expr)->expr.primary.kind = EXPR_PRIMARY_DECL;
                    (*expr)->expr.primary.primary.decl = decl     ;
                    break;

                case EXPR_PRIMARY_STRUCT:
                    expr_struct = (*expr)->expr.primary.primary.structt;
                    for (int i = 0; i < expr_struct.argc; ++i)
                    {
                        ExprPrimaryStructField f = *(expr_struct.argv[i]);
                        resolver_resolve_expr(resolver, &f.value);
                    }
                    break;

                default:
            }
            break;

        case EXPR_UNARY:
            resolver_resolve_expr(resolver, &(*expr)->expr.unary.unary);
            break;

        case EXPR_BINARY:
            resolver_resolve_expr(resolver, &(*expr)->expr.binary.left);
            resolver_resolve_expr(resolver, &(*expr)->expr.binary.right);
            break;

        case EXPR_FN:
            resolver_resolve_expr(resolver, &(*expr)->expr.fn.caller);
            for (int i = 0; i < (*expr)->expr.fn.argc; ++i)
            {
                Expr* e = (*expr)->expr.fn.argv[i];
                resolver_resolve_expr(resolver, &e);
            }
            break;

        default:
            fprintf(stderr, "[%s:%d] Variable resolution: Found expression of unknown kind.\n", __FILE__, __LINE__);
            exit(1);
    }

    // resolve_push_expr(resolver, *expr);
}

Type* resolver_resolve_type_expr(Resolver* resolver, TypeExpr* type_expr)
{
    Type   type;
    Type*  type_ptr;
    Type** polymorphic_argv = NULL;
    TypeExpr** type_expr_argv = NULL;
    TypeStructField** fields = NULL;
    char* identifier = NULL;
    int argc = 0;
    TypeExprStructField** argv = NULL;
    bool should_push_to_arena = true;
    Decl* decl = NULL;

    if (type_expr == NULL)
    {
        return resolver_create_type_variable(resolver);
    }

    switch (type_expr->kind)
    {
        case TYPE_EXPR_NIL:
            type = (Type)
            {
                .kind      = TYPE_NIL,
                .type.none = NULL    ,
            };
            break;

        case TYPE_EXPR_BOOL:
            type = (Type)
            {
                .kind      = TYPE_BOOL,
                .type.none = NULL     ,
            };
            break;

        case TYPE_EXPR_INT:
            type = (Type)
            {
                .kind      = TYPE_INT ,
                .type.none = NULL     ,
            };
            break;

        case TYPE_EXPR_REAL:
            type = (Type)
            {
                .kind      = TYPE_REAL,
                .type.none = NULL     ,
            };
            break;

        case TYPE_EXPR_STRING:
            type = (Type)
            {
                .kind      = TYPE_STRING,
                .type.none = NULL       ,
            };
            break;

        case TYPE_EXPR_LIST:
            type = (Type)
            {
                .kind = TYPE_LIST,
                .type.list = (TypeList)
                {
                    .type = resolver_resolve_type_expr(resolver, type_expr->type_expr.list.type),
                }
            };
            break;

        case TYPE_EXPR_STRUCT:
            argc = type_expr->type_expr.structt.argc;
            argv = type_expr->type_expr.structt.argv;

            for (int i = 0; i < argc; ++i) 
            {
                TypeExprStructField* f = argv[i];
                TypeStructField converted_field = (TypeStructField)
                {
                    .key   = f->key,
                    .value = resolver_resolve_type_expr(resolver, f->value),
                };
                TypeStructField* new_field = (TypeStructField*) arena_push(&resolver->arena, &converted_field, sizeof(TypeStructField));
                arrput(fields, new_field);
            }

            type = (Type)
            {
                .kind = TYPE_STRUCT,
                .type.structt = (TypeStruct)
                {
                    .field_num = argc,
                    .fields    = (TypeStructField**) arena_push(&resolver->arena, fields, argc * sizeof(TypeStructField*)),
                }
            };

            arrfree(fields);
            fields = NULL;
            break;

        case TYPE_EXPR_FN:
            type = (Type)
            {
                .kind = TYPE_FN,
                .type.fn = (TypeFn)
                {
                    .left  = resolver_resolve_type_expr(resolver, type_expr->type_expr.fn.left ),
                    .right = resolver_resolve_type_expr(resolver, type_expr->type_expr.fn.right),
                }
            };
            break;

        case TYPE_EXPR_IDENTIFIER:
            // In context, search for most recent declaration with said identifier
            // If a declaration is found, get it's respective type variable, and assign it to type.
            // Else, we create a new type variable declaration.
            identifier = type_expr->type_expr.identifier.identifier;

            decl = resolver_get_decl_by_identifier(resolver, identifier);
            if (decl != NULL)
            {
                if
                (
                    decl->kind == DECL_TYPE_VARIABLE
                    || decl->kind == DECL_ALIAS
                    || (decl->kind == DECL_TYPE && type_get_polymorphic_parameter_num(decl->type) == 0)
                )
                {
                    type_ptr = decl->type;
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
                    return NULL;
                }
            }
            else
            {
                decl = resolver_declare_type_variable(resolver, identifier);
                resolver_push_decl_to_context(resolver, decl);
                type_ptr = decl->type;
            }
            should_push_to_arena = false;

            break;

        case TYPE_EXPR_INSTANCE:
            identifier       = type_expr->type_expr.instance.caller;
            argc             = type_expr->type_expr.instance.argc  ;
            type_expr_argv   = type_expr->type_expr.instance.argv  ;

            decl = resolver_get_decl_by_identifier(resolver, identifier);
            if
                (
                    decl != NULL
                    && decl->kind == DECL_TYPE
                    && type_get_polymorphic_parameter_num(decl->type) == argc
                )
            {
                for (int i = 0; i < argc; ++i)
                {
                    TypeExpr* te = type_expr_argv[i];
                    Type*     t  = resolver_resolve_type_expr(resolver, te);
                    arrput(polymorphic_argv, t);
                }
                Type** tmp_ptr = polymorphic_argv;
                polymorphic_argv = (Type**) arena_push(&resolver->arena, polymorphic_argv, argc * sizeof(Type*));
                arrfree(tmp_ptr);

                type = (Type)
                {
                    .kind = TYPE_MONOMORPHIC,
                    .type.monomorphic = (TypeMonomorphic)
                    {
                        .polymorphic = decl->type,
                        .argc        = argc               ,
                        .argv        = polymorphic_argv   ,
                    }
                };
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
                return NULL;
            }

            break;

        default:
            assert(false);
    }

    if (should_push_to_arena)
    {
        type_ptr = (Type*) arena_push(&resolver->arena, &type, sizeof(Type));
        // arrput(resolver->types, type_ptr);
        return type_ptr;
    }
    else
    {
        return type_ptr;
    }
}

int  resolver_get_context_snapshot(Resolver* resolver)
{
    return arrlen(resolver->context);
}

void resolver_restore_context_snapshot(Resolver* resolver, int snapshot)
{
    int curr_snapshot = arrlen(resolver->context);

    assert(curr_snapshot >= snapshot);

    for (int i = 0; i < curr_snapshot - snapshot; ++i)
    {
        (void) arrpop(resolver->context);
    }
}

void resolver_push_decl_to_context(Resolver* resolver, Decl* decl)
{
    arrput(resolver->context, decl);
}


Type* resolver_resolve_stmt_fn_type(Resolver* resolver, StmtFn fn)
{
    Type* return_type = NULL;
    Type* node_ptr    = NULL;
    Type  type;

    TypeExpr* te = fn.return_type;
    Type    * t  = te != NULL
        ? resolver_resolve_type_expr(resolver, te)
        : resolver_create_type_variable(resolver);

    type = (Type)
    {
        .kind    = TYPE_FN,
        .type.fn = (TypeFn)
        {
            .left  = NULL,
            .right = t   ,
        }
    };
    return_type = (Type*) arena_push(&resolver->arena, &type, sizeof(Type));
    node_ptr = return_type;

    for (int i = 0; i < fn.argc; ++i)
    {
        TypeExpr* te = fn.argv[i]->type;
        Type    * t  = te != NULL
            ? resolver_resolve_type_expr(resolver, te)
            : resolver_create_type_variable(resolver);

        if (i == 0)
        {
            node_ptr->type.fn.left = t;
        }
        else
        {
            type = (Type)
            {
                .kind    = TYPE_FN,
                .type.fn = (TypeFn)
                {
                    .left  = t,
                    .right = node_ptr->type.fn.right,
                }
            };

            Type* tmp = (Type*) arena_push(&resolver->arena, &type, sizeof(Type));
            node_ptr->type.fn.right = tmp;
            node_ptr = tmp;
        }
    }

    return return_type;
}

char* resolver_get_existing_identifier(Resolver* resolver, char* identifier)
{
    int id_len = strlen(identifier);
    int len = arrlen(resolver->identifiers);
    char* new_id = NULL;

    for (int i = 0; i < len; ++i)
    {
        char* id = resolver->identifiers[i];
        if (memcmp(id, identifier, id_len) == 0)
        {
            return id;
        }
    }

    new_id = (char*) arena_push_empty(&resolver->arena, (id_len + 1) * sizeof(char));
    arrput(resolver->identifiers, new_id);

    memcpy(new_id, identifier, id_len);

    return new_id;
}

Decl* resolver_declare_let(Resolver* resolver, char* identifier, Type* type)
{
    Decl decl;
    Decl* decl_ptr;
    char* existing_identifier = resolver_get_existing_identifier(resolver, identifier);

    decl = (Decl)
    {
        .kind = DECL_LET ,
        .identifier = existing_identifier,
        .type       = type      ,
    };

    decl_ptr = (Decl*) arena_push(&resolver->arena, &decl, sizeof(Decl));
    arrput(resolver->declarations, decl_ptr);

    return decl_ptr;
}

void resolver_declare_fn_params(Resolver* resolver, StmtFn fn, Type* fn_type)
{
    assert(fn_type->kind == TYPE_FN);
    assert(fn.argc == 0 ? fn_type->type.fn.left == NULL : true);

    char* identifier = NULL;
    Type* type       = NULL;
    Type* node       = fn_type;
    Decl* decl       = NULL;

    for (int i = 0; i < fn.argc; ++i)
    {
        assert(node->kind == TYPE_FN);

        identifier = fn.argv[i]->identifier;

        type = node->type.fn.left;
        decl = resolver_declare_let(resolver, identifier, type);
        resolver_push_decl_to_context(resolver, decl);
        node = node->type.fn.right;
    }
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

int type_get_polymorphic_parameter_num(Type* type)
{
    assert(type->kind == TYPE_POLYMORPHIC);

    return type->type.polymorphic.parameter_num;
}

void resolver_throw_compiler_error(Resolver* resolver, CompileError err)
{
    CompileError* err_ptr = NULL;
    err_ptr = (CompileError*) arena_push(&resolver->arena, &err, sizeof(CompileError));
    arrput(resolver->errs, err_ptr);
}

Type* resolver_create_type_variable(Resolver* resolver)
{
    Type type;

    type = (Type)
    {
        .kind      = TYPE_VARIABLE,
        .type.none = NULL         ,
    };

    return (Type*) arena_push(&resolver->arena, &type, sizeof(Type));
}

Decl* resolver_declare_type_variable(Resolver* resolver, char* identifier)
{
    Decl decl;

    decl = (Decl)
    {
        .kind       = DECL_TYPE_VARIABLE,
        .identifier = identifier        ,
        .type       = resolver_create_type_variable(resolver)
    };

    return (Decl*) arena_push(&resolver->arena, &decl, sizeof(Decl));
}
