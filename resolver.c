#include "resolver.h"

Resolver resolver_init(Parser parser, Stmt* stmts)
{
    return (Resolver)
    {
        .txt             = parser.txt   ,
        .tokens          = parser.tokens,
        .stmts           = stmts        ,
        .arena           = parser.arena ,
        .tmp_type_arena  = NULL         ,
        .loop_depth      = 0            ,
        .inside_function = false        ,
        .fn_type         = NULL         ,
        .declarations    = NULL         ,
        .exprs           = NULL         ,
        .types           = NULL         ,
        .identifiers     = NULL         ,
        .errs            = NULL         ,
    };
}

void resolver_free(Resolver* resolver)
{
    free((char*)resolver->txt);
    arrfree(resolver->tokens);
    arena_free(&resolver->arena);
    arena_free(&resolver->tmp_type_arena);

    arrfree(resolver->declarations);
    arrfree(resolver->exprs       );
    arrfree(resolver->types       );
    arrfree(resolver->identifiers );
    arrfree(resolver->errs        );

    *resolver = (Resolver)
    {
        .txt             = NULL                    ,
        .tokens          = NULL                    ,
        .stmts           = NULL                    ,
        .arena           = resolver->arena         ,
        .tmp_type_arena  = resolver->tmp_type_arena,
        .loop_depth      = 0                       ,
        .inside_function = false                   ,
        .fn_type         = NULL                    ,
        .declarations    = NULL                    ,
        .exprs           = NULL                    ,
        .types           = NULL                    ,
        .identifiers     = NULL                    ,
        .errs            = NULL                    ,
    };
}

void resolver_resolve_stmt(Resolver* resolver)
{
    Stmt* curr_stmt  = NULL;

    char * identifier = NULL;
    Type * type       = NULL;
    Expr * expr       = NULL;
    Stmt * stmt       = NULL;
    int    size       = 0   ;
    Stmt** body       = NULL;
    StmtFn fn;

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

            for (int i = 0; i < size; ++i)
            {
                Stmt* stmt = body[0];

                resolver->stmts = stmt     ;
                resolver_resolve_stmt(resolver);
                resolver->stmts = curr_stmt;
            }
            break;

        case STMT_LET     :
            identifier = curr_stmt->stmt.let.identifier;
            type       = curr_stmt->stmt.let.type      ;
            expr       = curr_stmt->stmt.let.expr      ;

            resolver_declare_let(resolver, identifier, type);

            resolver_resolve_expr(resolver, expr, NULL);
            if (expr == NULL)
            {
                resolver->stmts = NULL;
                return;
            }
            break;

        case STMT_EXPR    :
            expr = curr_stmt->stmt.expr;

            resolver_resolve_expr(resolver, expr, NULL);
            if (expr == NULL)
            {
                resolver->stmts = NULL;
                return;
            }
            break;

        case STMT_IF      :
            expr = curr_stmt->stmt.iff.condition;
            stmt = curr_stmt->stmt.iff.body     ;

            resolver_resolve_expr(resolver, expr, NULL);
            if (expr == NULL)
            {
                resolver->stmts = NULL;
                return;
            }

            resolver->stmts = stmt     ;

            resolver_resolve_stmt(resolver);

            resolver->stmts = curr_stmt;
            break;

        case STMT_WHILE   :
            expr = curr_stmt->stmt.whilee.condition;
            stmt = curr_stmt->stmt.whilee.body     ;

            resolver_resolve_expr(resolver, expr, NULL);
            if (expr == NULL)
            {
                resolver->stmts = NULL;
                return;
            }

            resolver->stmts = stmt     ;
            resolver->loop_depth++;

            resolver_resolve_stmt(resolver);

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

        case STMT_FN      :
            fn = curr_stmt->stmt.fn;
            stmt = fn.body;

            Type* new_fn_type = resolver_declare_fn(resolver, fn)->decl.fn.variable->type; // What the hell is this?? xdddddd

            bool inside_function = resolver->inside_function;
            Type* fn_type = resolver->fn_type;

            resolver->stmts = stmt;
            resolver->inside_function = true;
            resolver->fn_type = new_fn_type;

            resolver_resolve_stmt(resolver);

            resolver->stmts = curr_stmt;
            resolver->inside_function = inside_function;
            resolver->fn_type = fn_type;
            break;

        case STMT_RETURN  :
            assert(curr_stmt->kind = STMT_RETURN);

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
                resolver_resolve_expr(resolver, expr, resolver->fn_type);
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

// Set variable to the biggest instance of identifier.
// Also, if type isn't NULL, bind expr.type to type.
// TODO: Refactor this rickety ass code.
void resolver_resolve_expr(Resolver* resolver, Expr* expr, Type* type)
{
    switch (expr->kind)
    {
        case EXPR_PRIMARY:
            // TODO: expand this later functions, once we implement them.
            if (expr->expr.primary.kind == EXPR_PRIMARY_IDENTIFIER)
            {
                char* identifier = expr->expr.primary.primary.identifier;
                Variable* var = NULL;

                var = resolver_get_nearest_variable(resolver, identifier);
                if (var == NULL)
                {
                    expr = NULL;
                    return;
                }

                expr->expr.primary.kind = EXPR_PRIMARY_VARIABLE;
                expr->expr.primary.primary.variable = var;
            }
            else if (expr->expr.primary.kind == EXPR_PRIMARY_STRUCT)
            {
                ExprPrimaryStruct expr_struct = expr->expr.primary.primary.structt;
                for (int i = 0; i < expr_struct.argc; ++i)
                {
                    ExprPrimaryStructField f = *(expr_struct.argv[i]);
                    resolver_resolve_expr(resolver, f.value, f.type);
                }
            }
            break;

        case EXPR_UNARY:
            resolver_resolve_expr(resolver, expr->expr.unary.unary, NULL);
            break;

        case EXPR_BINARY:
            resolver_resolve_expr(resolver, expr->expr.binary.left , NULL);
            resolver_resolve_expr(resolver, expr->expr.binary.right, NULL);
            break;

        case EXPR_FN:
            resolver_resolve_expr(resolver, expr->expr.fn.caller, NULL);
            for (int i = 0; i < expr->expr.fn.argc; ++i)
            {
                Expr* e = expr->expr.fn.argv[i];
                resolver_resolve_expr(resolver, e, NULL);
            }
            break;

        default:
            fprintf(stderr, "[%s:%d] Variable resolution: Found expression of unknown kind.\n", __FILE__, __LINE__);
            exit(1);
    }

    if (type != NULL)
    {
        expr->type = type;
    }
}

Decl* resolver_declare_let(Resolver* resolver, char* identifier, Type* type)
{
    Decl  decl;
    Decl* decl_ptr = NULL;

    Variable  var;
    Variable* var_ptr = NULL;

    char* existing_identifier = NULL;

    existing_identifier = resolver_get_identifier(resolver, identifier);

    var = (Variable)
    {
        .identifier = existing_identifier,
        .type       = type               ,
    };

    var_ptr = (Variable*) arena_push(&resolver->arena, &var, sizeof(Variable));

    decl = (Decl)
    {
        .kind = DECL_LET,
        .decl.let = (DeclLet)
        {
            .variable = var_ptr,
        }
    };

    decl_ptr = (Decl*) arena_push(&resolver->arena, &decl, sizeof(Decl));
    arrput(resolver->declarations, decl_ptr);

    return decl_ptr;
}

// TODO: Make it so that we cannot have function arguments that have the same identifier as the function name.
Decl* resolver_declare_fn(Resolver* resolver, StmtFn fn)
{
    Decl  decl;
    Decl* decl_ptr = NULL;

    Variable  var;
    Variable* var_ptr = NULL;

    Type* fn_type = NULL;

    int argc = 0;
    Variable** argv = NULL;

    char* existing_identifier = NULL;

    existing_identifier = resolver_get_identifier(resolver, fn.identifier);

    fn_type = construct_fn_type(&resolver->arena, fn);
    var = (Variable)
    {
        .identifier = existing_identifier,
        .type       = fn_type            ,
    };

    var_ptr = (Variable*) arena_push(&resolver->arena, &var, sizeof(Variable));

    for (int i = 0; i < fn.argc; ++i)
    {
        Decl* d    = NULL;
        char* arg  = NULL;
        Type* type_arg  = NULL;

        arg      = fn.argv[i]->identifier  ;
        type_arg = fn_type->type.fn.argv[i];
        d        = resolver_declare_let(resolver, arg, type_arg);
        assert(d->kind == DECL_LET);
        arrput(argv, d->decl.let.variable);
    }

    argc = arrlen(argv);
    Variable** tmp_ptr = argv;
    argv = (Variable**) arena_push(&resolver->arena, argv, argc * sizeof(Variable*));
    arrfree(tmp_ptr);

    decl = (Decl)
    {
        .kind = DECL_FN,
        .decl.fn = (DeclFn)
        {
            .variable = var_ptr,
            .argc     = argc   ,
            .argv     = argv   ,
        }
    };

    decl_ptr = (Decl*) arena_push(&resolver->arena, &decl, sizeof(Decl));
    arrput(resolver->declarations, decl_ptr);

    return decl_ptr;
}

char* resolver_get_identifier(Resolver* resolver, char* identifier)
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

    new_id = (char*) arena_push(&resolver->arena, identifier, id_len * sizeof(char));
    arrput(resolver->identifiers, new_id);

    return new_id;
}

Variable* resolver_get_nearest_variable(Resolver* resolver, char* identifier)
{
    Decl** declarations = resolver->declarations;
    int decl_number = arrlen(declarations);
    Variable* var = NULL;

    // We iterate in reverse - this is because the later elements are newer than earlier ones.
    // Therefore, if there are multiple variables with the same identifier,
    // then the ones that are closer to the end of the list are going to be the closest in scope.
    for (int i = decl_number - 1; 0 < i; --i)
    {
        Decl d = *(declarations[i]);
        switch (d.kind)
        {
            case DECL_LET:
                var = d.decl.let.variable;
                break;

            case DECL_FN:
                var = d.decl.fn.variable;
                break;

            default:
                fprintf(stderr, "[%s:%d] Variable resolution: Found declaration of unknown kind.\n", __FILE__, __LINE__);
                exit(1);
        }

        if (memcmp(var->identifier, identifier, strlen(identifier)) == 0)
        {
            return var;
        }
    }

    resolver_throw_compiler_error(resolver, (CompileError)
    {
        .kind   = ERROR_ERROR      ,
        .line   = -1               ,
        .column = -1               ,
        .length = -1               ,
        .msg    = "Variable resolution: Variable not declared.",
    });
    return NULL;
}

void resolver_throw_compiler_error(Resolver* resolver, CompileError err)
{
    CompileError* err_ptr = NULL;
    err_ptr = (CompileError*) arena_push(&resolver->arena, &err, sizeof(CompileError));
    arrput(resolver->errs, err_ptr);
}

Type* construct_fn_type(Arena* arena, StmtFn fn)
{
    Type type;
    Type** argv = NULL;

    for (int i = 0; i < fn.argc; ++i)
    {
        Type* t = fn.argv[i]->type;
        if (t == NULL)
        {
            Type new_type = (Type)
            {
                .kind   = TYPE_VARIABLE,
                .line   = -1           ,
                .column = -1           ,
                .length = -1           ,
                .type.none = NULL      ,
            };

            t = (Type*) arena_push(arena, &new_type, sizeof(Type));
        }

        arrput(argv, t);
    }

    Type** tmp_ptr = argv;
    argv = (Type**) arena_push(arena, argv, fn.argc * sizeof(Type*));
    arrfree(tmp_ptr);

    type = (Type)
    {
        .kind   = TYPE_FN,
        .line   = -1     ,
        .column = -1     ,
        .length = -1     ,

        .type.fn = (TypeFunction)
        {
            .argc = fn.argc,
            .argv = argv   , // Type**
        }
    };

    return (Type*) arena_push(arena, &type, sizeof(Type));
}
