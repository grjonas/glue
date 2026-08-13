#include "resolver.h"

static ResolverSnapshot resolver_get_context_snapshot    (Resolver* resolver);
static void     resolver_restore_context_snapshot(Resolver* resolver, ResolverSnapshot snapshot);
static void resolver_push_decl_to_context(Resolver* resolver, Decl* decl); 

static Decl* resolver_declare_variable            (Resolver* resolver, char* identifier);
static Decl* resolver_declare_type_variable       (Resolver* resolver, char* identifier);
static Decl* resolver_declare_alias               (Resolver* resolver, char* identifier);
static Decl* resolver_declare_new_type            (Resolver* resolver, char* identifier);
static Decl* resolver_declare_new_type_constructor(Resolver* resolver, char* identifier);

static char* resolver_get_existing_identifier (Resolver* resolver, char* identifier);
static Decl* resolver_get_decl_by_identifier  (Resolver* resolver, char* identifier);

static Span resolver_get_stmt_span      (Resolver* resolver, Stmt* stmt);
static Span resolver_get_expr_span      (Resolver* resolver, Expr* expr);
static Span resolver_get_type_expr_span (Resolver* resolver, TypeExpr* type_expr);
static Span resolver_get_pattern_span   (Resolver* resolver, Pattern* pattern);

static void resolver_throw_err_stmt_break_not_in_loop                 (Resolver* resolver, Stmt* stmt);
static void resolver_throw_err_stmt_continue_not_in_loop              (Resolver* resolver, Stmt* stmt);
static void resolver_throw_err_stmt_return_not_in_fn                  (Resolver* resolver, Stmt* stmt);
static void resolver_throw_err_stmt_returns_dont_match                (Resolver* resolver, Stmt* stmt);
static void resolver_throw_err_expr_failed_to_resolve_identifier      (Resolver* resolver, Expr* expr, char* identifier);
static void resolver_throw_err_type_expr_failed_to_resolve_identifier (Resolver* resolver, TypeExpr* type_expr, char* identifier);
static void resolver_throw_err_expr_failed_to_resolve_access_op       (Resolver* resolver, Expr* expr);
static void resolver_throw_err_type_expr_failed_to_find_type          (Resolver* resolver, TypeExpr* type_expr, char* identifier);
static void resolver_throw_err_pattern_failed_to_resolve_identifier   (Resolver* resolver, Pattern* pattern, const char* identifier);
static void resolver_throw_err_pattern_failed_to_find_constructor     (Resolver* resolver, Pattern* pattern, const char* identifier);

// TODO: Fix init and free for the resolver, because it doesn't really free everything (the errors mainly).
extern Resolver resolver_init(Parser* parser, Stmt* stmts)
{
    // Parser cleanup
    diagnostic_component_free(&parser->diagnostic_component);

    *parser = (Parser)
    {
        .filename = NULL                 ,
        .txt      = NULL                 ,
        .tokens   = NULL                 ,
        .start    = -1                   ,
        .end      = -1                   ,
        .current  = -1                   ,
        .diagnostic_component = NULL     ,
    };

    Resolver resolver = (Resolver)
    {
        .filename         = parser->filename,
        .txt              = parser->txt     ,
        .tokens           = parser->tokens  ,
        .stmts            = stmts           ,
        .arena            = parser->arena   ,
        .type_variable_id = 0               ,
        .decl_id          = 0               ,
        .loop_depth       = 0               ,
        // .inside_function  = false           ,
        .curr_fn          = NULL            ,
        .context          = NULL            ,
        .declarations     = NULL            ,
        .identifiers      = NULL            ,
        .diagnostic_component = diagnostic_component_init(),
    };
    return resolver;
}

extern void resolver_free(Resolver* resolver)
{
    free((char*)resolver->txt);
    arrfree(resolver->tokens);
    arena_free(&resolver->arena);

    arrfree(resolver->context     );
    arrfree(resolver->declarations);
    arrfree(resolver->identifiers );
    diagnostic_component_free(&resolver->diagnostic_component);

    *resolver = (Resolver)
    {
        .txt              = NULL           ,
        .tokens           = NULL           ,
        .stmts            = NULL           ,
        .arena            = resolver->arena,
        .type_variable_id = 0              ,
        .decl_id          = 0              ,
        .loop_depth       = 0              ,
        // .inside_function  = false          ,
        .curr_fn          = NULL           ,
        .context          = NULL           ,
        .declarations     = NULL           ,
        .identifiers      = NULL           ,
        .diagnostic_component = NULL      ,
    };
}

static bool resolver_resolve_stmt_block(Resolver* resolver)
{
    Stmt* curr_stmt  = resolver->stmts;

    int    size = 0   ;
    Stmt** body = NULL;
    ResolverSnapshot snapshot ;

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

static bool resolver_resolve_stmt_let(Resolver* resolver)
{
    Stmt* curr_stmt  = resolver->stmts;

    char    * identifier = NULL;
    TypeExpr* type_expr  = NULL;
    Expr    * expr       = NULL;
    Decl    * decl       = NULL;
    ResolverSnapshot snapshot;

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

static bool resolver_resolve_stmt_expr(Resolver* resolver)
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

static bool resolver_resolve_stmt_if(Resolver* resolver)
{
    Stmt* curr_stmt  = resolver->stmts;

    Expr* expr = NULL;
    Stmt* stmt = NULL;
    Stmt* next = NULL;
    ResolverSnapshot snapshot;

    expr = curr_stmt->stmt.iff.condition;
    stmt = curr_stmt->stmt.iff.body     ;
    next = curr_stmt->stmt.iff.next     ;

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

    if (next != NULL)
    {
        resolver->stmts = next;
        if (!resolver_resolve_stmt_if(resolver))
        {
            return false;
        }
    }
    resolver->stmts = curr_stmt;

    return true;
}

static bool resolver_resolve_stmt_while(Resolver* resolver)
{
    Stmt* curr_stmt  = resolver->stmts;

    Expr* expr = NULL;
    Stmt* stmt = NULL;
    ResolverSnapshot snapshot;

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

static bool resolver_resolve_stmt_break(Resolver* resolver)
{
    Stmt* curr_stmt  = resolver->stmts;

    if (resolver->loop_depth <= 0)
    {
        resolver_throw_err_stmt_break_not_in_loop(resolver, curr_stmt);
        return false;
    }

    return true;
}

static bool resolver_resolve_stmt_continue(Resolver* resolver)
{
    Stmt* curr_stmt  = resolver->stmts;

    if (resolver->loop_depth <= 0)
    {
        resolver_throw_err_stmt_continue_not_in_loop(resolver, curr_stmt);
        return false;
    }

    return true;
}

static bool resolver_resolve_stmt_fn(Resolver* resolver)
{
    Stmt* curr_stmt  = resolver->stmts;

    StmtFn* fn;
    Stmt*    stmt = NULL;
    Decl*    decl = NULL;
    Decl* curr_fn = NULL;
    ResolverSnapshot snapshot;

    fn   = &curr_stmt->stmt.fn;
    stmt = fn->body;

    // Does not return NULL
    decl = resolver_declare_variable(resolver, fn->identifier);
    fn->decl = decl;
    resolver_push_decl_to_context(resolver, decl);

    snapshot = resolver_get_context_snapshot(resolver);

    if (!resolver_resolve_fn_args(resolver, fn->argc, fn->argv, fn->return_type))
    {
        return false;
    }

    curr_fn = resolver->curr_fn;
    resolver->stmts = stmt;
    resolver->curr_fn = decl;

    // Assigning types to argument.
    if (!resolver_resolve_stmt(resolver))
    {
        return false;
    }

    resolver->stmts = curr_stmt;
    resolver->curr_fn = curr_fn;

    resolver_restore_context_snapshot(resolver, snapshot);

    return true;
}

static bool resolver_resolve_stmt_return(Resolver* resolver)
{
    Stmt* curr_stmt  = resolver->stmts;

    Expr* expr = NULL;
    Decl* curr_fn = resolver->curr_fn;

    assert(curr_stmt != NULL);
    assert(curr_stmt->kind == STMT_RETURN);

    // TODO: Somehow bind this to it's respective function declaration.
    if (curr_fn == NULL)
    {
        resolver_throw_err_stmt_return_not_in_fn(resolver, curr_stmt);
        return false;
    }
    assert(curr_fn->kind == DECL_VAR);

    expr = curr_stmt->stmt.returnn.expr;
    if (expr != NULL)
    {
        switch (curr_fn->decl.var.return_kind)
        {
            case DECL_VAR_RETURN_NONE    :
                curr_fn->decl.var.return_kind = DECL_VAR_RETURN_NOT_NULL; break;
            case DECL_VAR_RETURN_NULL    :
                resolver_throw_err_stmt_returns_dont_match(resolver, curr_stmt);
                return false;
            case DECL_VAR_RETURN_NOT_NULL: break;
        }

        if (!resolver_resolve_expr(resolver, expr))
        {
            return false;
        }
    }
    else
    {
        switch (curr_fn->decl.var.return_kind)
        {
            case DECL_VAR_RETURN_NONE    :
                curr_fn->decl.var.return_kind = DECL_VAR_RETURN_NULL; break;
            case DECL_VAR_RETURN_NULL    :
                resolver_throw_err_stmt_returns_dont_match(resolver, curr_stmt);
                return false;
            case DECL_VAR_RETURN_NOT_NULL: break;
        }
    }
    curr_stmt->stmt.returnn.decl = curr_fn;

    return true;
}

static bool resolver_resolve_stmt_alias(Resolver* resolver)
{
    Stmt* curr_stmt  = resolver->stmts;

    Decl    * decl       = NULL;
    char    * identifier = NULL;
    TypeExpr* type_expr  = NULL;

    ResolverSnapshot snapshot;

    assert(curr_stmt != NULL);
    assert(curr_stmt->kind == STMT_ALIAS);

    identifier = curr_stmt->stmt.alias.identifier;
    type_expr  = curr_stmt->stmt.alias.type      ;

    decl = resolver_declare_alias(resolver, identifier);
    resolver_push_decl_to_context(resolver, decl);

    decl->decl.alias.type_expr = type_expr;

    snapshot = resolver_get_context_snapshot(resolver);

    if (!resolver_resolve_type_expr(resolver, type_expr))
    {
        return false;
    }

    resolver_restore_context_snapshot(resolver, snapshot);

    curr_stmt->stmt.alias.decl = decl;

    return true;
}

static void resolver_declare_type_constructors_for_type
    (Resolver* resolver, int constructor_num, StmtTypeConstructor** constructors,
        DYNAMIC_ARRAY(Decl**)* type_constructors_ref)
{
    for (int i = 0; i < constructor_num; ++i)
    {
        StmtTypeConstructor* stmt_type_constructor = constructors[i];
        Decl* type_constructor =
            resolver_declare_new_type_constructor
                (resolver, stmt_type_constructor->identifier);
        resolver_push_decl_to_context(resolver, type_constructor);

        type_constructor->decl.constructor = (DeclTypeConstructor)
        {
            .types    = stmt_type_constructor->types   ,
            .type_num = stmt_type_constructor->type_num,
        };

        arrput(*type_constructors_ref, type_constructor);
    }
}

static void resolver_declare_type_variables_for_type
    (Resolver* resolver, int argc, char** argv, DYNAMIC_ARRAY(Decl**)* type_vars_ref)
{
    for (int i = 0; i < argc; ++i)
    {
        Decl* type_var = resolver_declare_type_variable(resolver, argv[i]);
        resolver_push_decl_to_context(resolver, type_var);

        arrput(*type_vars_ref,  type_var);
    }
}

static bool resolver_resolve_stmt_type_constructors
    (Resolver* resolver, int constructor_num, StmtTypeConstructor** constructors)
{
    for (int i = 0; i < constructor_num; ++i)
    {
        StmtTypeConstructor* stmt_type_constructor = constructors[i];
        for (int j = 0; j < stmt_type_constructor->type_num; ++j)
        {
            if (!resolver_resolve_type_expr(resolver, stmt_type_constructor->types[j]))
            {
                return false;
            }
        }
    }

    return true;
}

static bool resolver_resolve_stmt_type(Resolver* resolver)
{
    Stmt* curr_stmt  = resolver->stmts;

    char               *  identifier      = NULL;
    char               ** argv            = NULL;
    StmtTypeConstructor** constructors    = NULL;
    int                   argc            = 0   ;
    int                   constructor_num = 0   ;

    Decl               *  decl            = NULL;

    Decl** type_vars         = NULL;
    Decl** type_constructors = NULL;

    ResolverSnapshot snapshot;

    assert(curr_stmt != NULL);
    assert(curr_stmt->kind == STMT_TYPE);

    identifier      = curr_stmt->stmt.type.identifier     ;
    argv            = curr_stmt->stmt.type.argv           ;
    constructors    = curr_stmt->stmt.type.constructors   ;
    argc            = curr_stmt->stmt.type.argc           ;
    constructor_num = curr_stmt->stmt.type.constructor_num;

    decl = resolver_declare_new_type(resolver, identifier);
    resolver_push_decl_to_context(resolver, decl);

    // Declaration of type constructors - it is important that they are declared
    // before taking the snapshot -
    // otherwise they will only be valid withing scope of type declaration.
    // Actual resolution of them is done later - after declaring variables
    resolver_declare_type_constructors_for_type
        (resolver, constructor_num, constructors, &type_constructors);

    Decl** tmp_cons = type_constructors;
    type_constructors = (Decl**) arena_push(&resolver->arena, type_constructors, constructor_num * sizeof(Decl*));
    arrfree(tmp_cons);

    snapshot = resolver_get_context_snapshot(resolver);

    // Variable declaration - done after taking the snapshot,
    // such that they are not valid outside the scope of the type statement.
    resolver_declare_type_variables_for_type
        (resolver, argc, argv, &type_vars);

    Decl** tmp_vars = type_vars;
    type_vars = (Decl**) arena_push(&resolver->arena, type_vars, argc * sizeof(Decl*));
    arrfree(tmp_vars);

    // Type constructor resolution
    if (!resolver_resolve_stmt_type_constructors(resolver, constructor_num, constructors))
    {
        return false;
    }

    resolver_restore_context_snapshot(resolver, snapshot);

    decl->decl.type = (DeclType)
    {
        .type_vars       = type_vars        ,
        .constructors    = type_constructors,
        .type_var_num    = argc             ,
        .constructor_num = constructor_num  ,
    };
    curr_stmt->stmt.type.decl = decl;

    return true;
}

static bool resolver_resolve_stmt_match(Resolver* resolver)
{
    Stmt* curr_stmt = resolver->stmts;

    assert(curr_stmt != NULL);
    assert(curr_stmt->kind == STMT_MATCH);

    Expr*  scrutinee = curr_stmt->stmt.match.scrutinee;
    StmtCase** cases = curr_stmt->stmt.match.cases    ;
    int     case_num = curr_stmt->stmt.match.case_num ;

    ResolverSnapshot snapshot;

    if (!resolver_resolve_expr(resolver, scrutinee))
    {
        return false;
    }

    for (int i = 0; i < case_num; ++i)
    {
        StmtCase* casee = cases[i];

        if (!resolver_resolve_pattern(resolver, casee->pattern))
        {
            return false;
        }

        snapshot = resolver_get_context_snapshot(resolver);
        resolver->stmts = casee->stmt;
        if (!resolver_resolve_stmt(resolver))
        {
            return false;
        }
        resolver_restore_context_snapshot(resolver, snapshot);
    }

    resolver->stmts = curr_stmt;
    return true;
}

// On error - stmts is set to NULL.
extern bool resolver_resolve_stmt(Resolver* resolver)
{
    Stmt* curr_stmt  = NULL;

    curr_stmt = resolver->stmts;
    if (curr_stmt == NULL)
    {
        fprintf(stderr, "[%s:%d] Variable resolution: Found NULL statement.\n", __FILE__, __LINE__);
        exit(1);
    }

    switch (curr_stmt->kind)
    {
        case STMT_BLOCK   : return resolver_resolve_stmt_block   (resolver);
        case STMT_LET     : return resolver_resolve_stmt_let     (resolver);
        case STMT_EXPR    : return resolver_resolve_stmt_expr    (resolver);
        case STMT_IF      : return resolver_resolve_stmt_if      (resolver);
        case STMT_WHILE   : return resolver_resolve_stmt_while   (resolver);
        case STMT_BREAK   : return resolver_resolve_stmt_break   (resolver);
        case STMT_CONTINUE: return resolver_resolve_stmt_continue(resolver);
        case STMT_FN      : return resolver_resolve_stmt_fn      (resolver);
        case STMT_RETURN  : return resolver_resolve_stmt_return  (resolver);
        case STMT_ALIAS   : return resolver_resolve_stmt_alias   (resolver);
        case STMT_TYPE    : return resolver_resolve_stmt_type    (resolver);
        case STMT_MATCH   : return resolver_resolve_stmt_match   (resolver);

        default:
            fprintf(stderr, "[%s:%d] Variable resolution: Found statement of unknown kind.\n", __FILE__, __LINE__);
            exit(1);
    }

    resolver->stmts = curr_stmt;

    return true;
}

static bool resolver_resolve_expr_primary_lambda(Resolver* resolver, Expr* expr)
{
    Stmt* curr_stmt = resolver->stmts;

    ExprPrimaryLambda* lambda = NULL;
    Stmt*    stmt = NULL;
    Decl*    decl = NULL;
    Decl* curr_fn = NULL;
    ResolverSnapshot snapshot;

    lambda = &expr->expr.primary.primary.lambda;
    stmt   = lambda->body;

    // This is hacky asf, but it works, and that is what matters :)
    decl = resolver_declare_variable(resolver, "");
    lambda->decl = decl;
    resolver_push_decl_to_context(resolver, decl);

    snapshot = resolver_get_context_snapshot(resolver);

    if (!resolver_resolve_fn_args(resolver, lambda->argc, lambda->argv, lambda->return_type))
    {
        return false;
    }

    curr_fn = resolver->curr_fn;
    resolver->stmts = stmt;
    resolver->curr_fn = decl;

    // Assigning types to argument.
    if (!resolver_resolve_stmt(resolver))
    {
        return false;
    }

    resolver->stmts = curr_stmt;
    resolver->curr_fn = curr_fn;

    resolver_restore_context_snapshot(resolver, snapshot);

    return true;
}

// TODO: expand this later to functions, once we implement them.
static bool resolver_resolve_expr_primary(Resolver* resolver, Expr* expr)
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
            else if (decl->kind != DECL_VAR && decl->kind != DECL_TYPE_CONSTRUCTOR)
            {
                resolver_throw_err_expr_failed_to_resolve_identifier(resolver, expr, identifier);
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

        case EXPR_PRIMARY_LAMBDA:
            resolver_resolve_expr_primary_lambda(resolver, expr);
            break;

        default:
    }

    return true;
}

static bool resolver_resolve_expr_binary_access_operator(Resolver* resolver, Expr* expr)
{
    assert(expr != NULL);

    // if   expression is an identifier, then return true
    // elif
    //     expression is a binary operator
    //     , and  the operator type is access
    //     , and  the left hand-side is an identifier
    //     , then recursively call function on the right-hand side
    // else return false

    // Beautiful code :)))
    if (expr->kind == EXPR_PRIMARY && expr->expr.primary.kind == EXPR_PRIMARY_IDENTIFIER)
    {
        char** identifier = &expr->expr.primary.primary.identifier;
        *identifier =
            resolver_get_existing_identifier(resolver, *identifier);
        return true;
    }
    // Also beautiful code :DDDDDDDD
    else if
        (
               expr->kind == EXPR_BINARY
            && expr->expr.binary.kind == EXPR_BINARY_ACCESS
            && expr->expr.binary.left->kind == EXPR_PRIMARY
            && expr->expr.binary.left->expr.primary.kind == EXPR_PRIMARY_IDENTIFIER
        )
    {
        char** identifier =
            &expr->expr.binary.left->expr.primary.primary.identifier;
        *identifier =
            resolver_get_existing_identifier(resolver, *identifier);
        return resolver_resolve_expr_binary_access_operator
            (resolver, expr->expr.binary.right);
    }
    else
    {
        resolver_throw_err_expr_failed_to_resolve_access_op(resolver, expr);
        return false;
    }
}

// 1) If type != NULL, then we bind then the type of the expression is equal to type.
// 2) Set variable to the most recent instance of identifier.
// TODO: Once we begin implementing inference, update this code.
extern bool resolver_resolve_expr(Resolver* resolver, Expr* expr)
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
            // We have to handle assignment properly.
            if (
                   expr->expr.binary.kind == EXPR_BINARY_ASSIGN
                   &&
                   (
                       expr->expr.binary.left->kind != EXPR_PRIMARY
                       || expr->expr.binary.left->expr.primary.kind != EXPR_PRIMARY_IDENTIFIER
                   )
               )
            {
                return false;
            }

            result = resolver_resolve_expr(resolver, expr->expr.binary.left);
            if (!result)
            {
                return false;
            }

            // The access operator has to be handled differently than other operators - we want to preserve the identifier on the right-hand side.
            if (expr->expr.binary.kind == EXPR_BINARY_ACCESS)
            {
                result = resolver_resolve_expr_binary_access_operator(resolver, expr->expr.binary.right);
                if (!result)
                {
                    return false;
                }
            }
            else
            {
                result = resolver_resolve_expr(resolver, expr->expr.binary.right);
                if (!result)
                {
                    return false;
                }
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

    identifier = type_expr->type_expr.identifier.identifier; 
    decl       = resolver_get_decl_by_identifier(resolver, identifier);
    if (decl != NULL)
    {
        if (decl_is_type_variable(*decl))
        {
            *type_expr = (TypeExpr)
            {
                .kind = TYPE_EXPR_VARIABLE,
                .type_expr.variable = (TypeExprVariable) { .decl = decl }
            };
        }
        else if (decl_is_alias(*decl))
        {
            *type_expr = (TypeExpr)
            {
                .kind = TYPE_EXPR_ALIAS,
                .type_expr.alias = (TypeExprAlias) { .decl = decl }
            };
        }
        else if (decl_is_new_type(*decl) && decl_get_new_type_parameter_num(*decl) == 0)
        {
            *type_expr = (TypeExpr)
            {
                .kind = TYPE_EXPR_APPLICATION,
                .type_expr.application = (TypeExprApplication)
                {
                    .decl = decl,
                    .argv = NULL,
                    .argc = 0   ,
                },
            };
        }
        else
        {
            resolver_throw_err_type_expr_failed_to_resolve_identifier(resolver, type_expr, identifier);
            return false;
        }
    }
    else
    {
        decl = resolver_declare_type_variable(resolver, identifier);
        resolver_push_decl_to_context(resolver, decl);

        type_expr->kind = TYPE_EXPR_VARIABLE;
        type_expr->type_expr.variable.decl = decl;
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

        type_expr->kind = TYPE_EXPR_APPLICATION;
        type_expr->type_expr.application.decl = decl;
        type_expr->type_expr.application.argv = argv;
        type_expr->type_expr.application.argc = argc;

        return true;
    }
    else
    {
        resolver_throw_err_type_expr_failed_to_find_type(resolver, type_expr, caller);
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
        case TYPE_EXPR_NAT       : return true;
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

        case TYPE_EXPR_APPLICATION:
            for (int i = 0; i < type_expr->type_expr.application.argc; ++i)
            {
                TypeExpr* te = type_expr->type_expr.application.argv[i];
                if (!resolver_resolve_type_expr(resolver, te))
                {
                    return false;
                }
            }
            return true;

        // Similiar to TYPE_EXPR_VARIABLE, shouldn't really be encountered in this stage.
        case TYPE_EXPR_ALIAS      : UNREACHABLE;
    }

    fprintf(stderr, "[%s:%d] Type expression resolution: Failed to recognise type expression kind.\n", __FILE__, __LINE__);
    exit(1);
} 

bool resolver_resolve_pattern_var(Resolver* resolver, Pattern* pattern)
{
    assert(pattern != NULL);
    assert(pattern->kind == PATTERN_VAR);

    const char* identifier = NULL;
    Decl      * decl       = NULL;

    identifier = pattern->pattern.var.var; 
    decl       = resolver_get_decl_by_identifier(resolver, (char*) identifier);
    if (decl != NULL)
    {
        if (decl_is_variable(*decl))
        {
            *pattern = (Pattern)
            {
                .kind = PATTERN_RESOLVED_VAR,
                .pattern.resolved_var = (PatternResolvedVar) { .decl = decl }
            };
        }
        else
        {
            resolver_throw_err_pattern_failed_to_resolve_identifier(resolver, pattern, identifier);
            return false;
        }
    }
    else
    {
        decl = resolver_declare_variable(resolver, (char*) identifier);
        resolver_push_decl_to_context(resolver, decl);

        pattern->kind = PATTERN_RESOLVED_VAR;
        pattern->pattern.resolved_var.decl = decl;
    }
    return true;
}

bool resolver_resolve_pattern_constructor(Resolver* resolver, Pattern* pattern)
{
    assert(pattern != NULL);
    assert(pattern->kind == PATTERN_CONSTRUCTOR);

    const char* identifier = NULL;
    int         argc   = 0   ;
    Pattern  ** argv   = NULL;
    Decl      * decl   = NULL;

    identifier = pattern->pattern.constructor.identifier;
    argc       = pattern->pattern.constructor.argc      ;
    argv       = pattern->pattern.constructor.argv      ;

    decl = resolver_get_decl_by_identifier(resolver, (char*) identifier);
    if
        (
            decl != NULL
            && decl_is_type_constructor(*decl)
            && decl_get_type_constructor_parameter_num(*decl) == argc
        )
    {
        for (int i = 0; i < argc; ++i)
        {
            Pattern* p = argv[i];
            bool result = resolver_resolve_pattern(resolver, p);
            if (!result)
            {
                return false;
            }
        }

        pattern->kind = PATTERN_APPLICATION;
        pattern->pattern.application.decl = decl;
        pattern->pattern.application.argv = argv;
        pattern->pattern.application.argc = argc;

        return true;
    }
    else
    {
        resolver_throw_err_pattern_failed_to_find_constructor(resolver, pattern, identifier);
        return false;
    }
}

bool resolver_resolve_pattern(Resolver* resolver, Pattern* pattern)
{
    assert(pattern != NULL);

    switch (pattern->kind)
    {
        case PATTERN_WILDCARD    : return true;
        case PATTERN_LITERAL     : return true;
        case PATTERN_VAR         :
            return resolver_resolve_pattern_var(resolver, pattern);
        case PATTERN_CONSTRUCTOR:
            return resolver_resolve_pattern_constructor(resolver, pattern);;

        case PATTERN_RESOLVED_VAR: UNREACHABLE;
        case PATTERN_APPLICATION : UNREACHABLE;
    }

    UNREACHABLE;
}

bool resolver_resolve_fn_args(Resolver* resolver, int argc, FnArg** argv, TypeExpr* return_type)
{
    if (!resolver_resolve_type_expr(resolver, return_type))
    {
        return false;
    }

    for (int i = 0; i < argc; ++i)
    {
        FnArg* arg = argv[i];

        arg->decl = resolver_declare_variable(resolver, arg->identifier);
        resolver_push_decl_to_context(resolver, arg->decl);

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

static ResolverSnapshot resolver_get_context_snapshot(Resolver* resolver)
{
    return (ResolverSnapshot)
    {
        .context_length   = arrlen(resolver->context) ,
        // .decl_id          = resolver->decl_id         ,
        // .type_variable_id = resolver->type_variable_id,
    };
}

static void resolver_restore_context_snapshot(Resolver* resolver, ResolverSnapshot snapshot)
{
    ResolverSnapshot curr_snapshot  = (ResolverSnapshot)
    {
        .context_length      = arrlen(resolver->context) ,
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

static void resolver_push_decl_to_context(Resolver* resolver, Decl* decl)
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
            LOG(LOG_DEBUG, "Reusing existing identifier: \"%s\"", id);
            return id;
        }
    }

    LOG(LOG_DEBUG, "Allocating new identifier  : \"%s\"", identifier);

    new_id = (char*) arena_push_empty(&resolver->arena, (id_len + 1) * sizeof(char));
    arrput(resolver->identifiers, new_id);

    memcpy(new_id, identifier, id_len);

    return new_id;
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
        id   = decl->identifier;

        if (strlen(id) == id_len && memcmp(id, identifier, id_len) == 0)
        {
            return decl;
        }
    }

    return NULL;
}

static Decl* resolver_declare_variable(Resolver* resolver, char* identifier)
{
    Decl decl;
    Decl* decl_ptr;
    char* existing_identifier = resolver_get_existing_identifier(resolver, identifier);

    decl = (Decl)
    {
        .kind       = DECL_VAR           ,
        .identifier = existing_identifier,
        .id         = resolver->decl_id++,
        .decl.var = (DeclVar)
        {
            .type   = NULL,
            .scheme = NULL,
            .return_kind = DECL_VAR_RETURN_NONE,
        }
    };

    decl_ptr = (Decl*) arena_push(&resolver->arena, &decl, sizeof(Decl));
    arrput(resolver->declarations, decl_ptr);

    return decl_ptr;
}

static Decl* resolver_declare_type_variable(Resolver* resolver, char* identifier)
{
    Decl decl;
    Decl* decl_ptr;
    char* existing_identifier = resolver_get_existing_identifier(resolver, identifier);

    decl = (Decl)
    {
        .kind       = DECL_TYPE_VAR      ,
        .identifier = existing_identifier,
        .id         = resolver->decl_id++,
        .decl.type_var = (DeclTypeVar)
        {
            .type = NULL,
        }
    };

    decl_ptr = (Decl*) arena_push(&resolver->arena, &decl, sizeof(Decl));
    arrput(resolver->declarations, decl_ptr);

    return decl_ptr;
}

static Decl* resolver_declare_alias(Resolver* resolver, char* identifier)
{
    Decl  decl;
    Decl* decl_ptr;
    char* existing_identifier = resolver_get_existing_identifier(resolver, identifier);

    decl = (Decl)
    {
        .kind       = DECL_ALIAS         ,
        .id         = resolver->decl_id++,
        .identifier = existing_identifier,
    };

    decl_ptr = (Decl*) arena_push(&resolver->arena, &decl, sizeof(Decl));
    arrput(resolver->declarations, decl_ptr);

    return decl_ptr;
}

static Decl* resolver_declare_new_type(Resolver* resolver, char* identifier)
{
    Decl decl;
    Decl* decl_ptr;
    char* existing_identifier = resolver_get_existing_identifier(resolver, identifier);

    decl = (Decl)
    {
        .kind       = DECL_TYPE          ,
        .identifier = existing_identifier,
        .id         = resolver->decl_id++,
    };

    decl_ptr = (Decl*) arena_push(&resolver->arena, &decl, sizeof(Decl));
    arrput(resolver->declarations, decl_ptr);

    return decl_ptr;
}

static Decl* resolver_declare_new_type_constructor(Resolver* resolver, char* identifier)
{
    Decl  decl;
    Decl* decl_ptr;
    char* existing_identifier = resolver_get_existing_identifier(resolver, identifier);

    decl = (Decl)
    {
        .kind       = DECL_TYPE_CONSTRUCTOR,
        .identifier = existing_identifier  ,
        .id         = resolver->decl_id++  ,
    };

    decl_ptr = (Decl*) arena_push(&resolver->arena, &decl, sizeof(Decl));
    arrput(resolver->declarations, decl_ptr);

    return decl_ptr;
}

static Span resolver_get_stmt_span(Resolver* resolver, Stmt* stmt)
{
    assert(resolver != NULL);
    assert(stmt     != NULL);

    return (Span)
    {
        .filename = resolver->filename,
        .line     = stmt->line        ,
        .column   = stmt->column      ,
        .length   = stmt->length      ,
    };
}

static Span resolver_get_expr_span(Resolver* resolver, Expr* expr)
{
    assert(resolver != NULL);
    assert(expr     != NULL);

    return (Span)
    {
        .filename = resolver->filename,
        .line     = expr->line        ,
        .column   = expr->column      ,
        .length   = expr->length      ,
    };
}

static Span resolver_get_type_expr_span(Resolver* resolver, TypeExpr* type_expr)
{
    assert(resolver  != NULL);
    assert(type_expr != NULL);

    return (Span)
    {
        .filename = resolver->filename,
        .line     = type_expr->line   ,
        .column   = type_expr->column ,
        .length   = type_expr->length ,
    };
}

static Span resolver_get_pattern_span(Resolver* resolver, Pattern* pattern)
{
    assert(resolver != NULL);
    assert(pattern  != NULL);

    return (Span)
    {
        .filename = resolver->filename,
        .line     = pattern->line   ,
        .column   = pattern->column ,
        .length   = pattern->length ,
    };
}

static void resolver_throw_err_stmt_break_not_in_loop(Resolver* resolver, Stmt* stmt)
{
    assert(resolver != NULL);
    assert(stmt     != NULL);

    diagnostic_component_push_err(resolver->diagnostic_component, (DiagnosticErr)
    {
        .kind = DIAGNOSTIC_ERR_BREAK_NOT_IN_LOOP,
        .span = resolver_get_stmt_span(resolver, stmt),
    });
}

static void resolver_throw_err_stmt_continue_not_in_loop(Resolver* resolver, Stmt* stmt)
{
    assert(resolver != NULL);
    assert(stmt     != NULL);

    diagnostic_component_push_err(resolver->diagnostic_component, (DiagnosticErr)
    {
        .kind = DIAGNOSTIC_ERR_CONTINUE_NOT_IN_LOOP,
        .span = resolver_get_stmt_span(resolver, stmt),
    });
}

static void resolver_throw_err_stmt_return_not_in_fn(Resolver* resolver, Stmt* stmt)
{
    assert(resolver != NULL);
    assert(stmt     != NULL);

    diagnostic_component_push_err(resolver->diagnostic_component, (DiagnosticErr)
    {
        .kind = DIAGNOSTIC_ERR_RETURN_NOT_IN_FN,
        .span = resolver_get_stmt_span(resolver, stmt),
    });
}

static void resolver_throw_err_stmt_returns_dont_match(Resolver* resolver, Stmt* stmt)
{
    assert(resolver != NULL);
    assert(stmt     != NULL);

    diagnostic_component_push_err(resolver->diagnostic_component, (DiagnosticErr)
    {
        .kind = DIAGNOSTIC_ERR_RETURNS_DONT_MATCH,
        .span = resolver_get_stmt_span(resolver, stmt),
    });
}

static void resolver_throw_err_expr_failed_to_resolve_identifier(Resolver* resolver, Expr* expr, char* identifier)
{
    assert(resolver != NULL);
    assert(expr     != NULL);
    assert(identifier != NULL);

    diagnostic_component_push_err(resolver->diagnostic_component, (DiagnosticErr)
    {
        .kind = DIAGNOSTIC_ERR_FAILED_TO_RESOLVE_IDENTIFIER,
        .span = resolver_get_expr_span(resolver, expr),
        .err.failed_to_resolve_identifier =
        {
            .identifier = diagnostic_component_add_identifier
                (resolver->diagnostic_component, identifier, strlen(identifier))
        }
    });
}

static void resolver_throw_err_type_expr_failed_to_resolve_identifier(Resolver* resolver, TypeExpr* type_expr, char* identifier)
{
    assert(resolver   != NULL);
    assert(type_expr  != NULL);
    assert(identifier != NULL);

    diagnostic_component_push_err(resolver->diagnostic_component, (DiagnosticErr)
    {
        .kind = DIAGNOSTIC_ERR_FAILED_TO_RESOLVE_IDENTIFIER,
        .span = resolver_get_type_expr_span(resolver, type_expr),
        .err.failed_to_resolve_identifier =
        {
            .identifier = diagnostic_component_add_identifier
                (resolver->diagnostic_component, identifier, strlen(identifier))
        }
    });
}

static void resolver_throw_err_expr_failed_to_resolve_access_op(Resolver* resolver, Expr* expr)
{
    assert(resolver != NULL);
    assert(expr     != NULL);

    diagnostic_component_push_err(resolver->diagnostic_component, (DiagnosticErr)
    {
        .kind = DIAGNOSTIC_ERR_FAILED_TO_RESOLVE_ACCESS_OP,
        .span = resolver_get_expr_span(resolver, expr),
    });
}

static void resolver_throw_err_type_expr_failed_to_find_type(Resolver* resolver, TypeExpr* type_expr, char* identifier)
{
    assert(resolver  != NULL);
    assert(type_expr != NULL);
    assert(identifier != NULL);

    diagnostic_component_push_err(resolver->diagnostic_component, (DiagnosticErr)
    {
        .kind = DIAGNOSTIC_ERR_FAILED_TO_FIND_TYPE,
        .span = resolver_get_type_expr_span(resolver, type_expr),
        .err.type_expr_failed_to_find_type =
        {
            .identifier = diagnostic_component_add_identifier
                (resolver->diagnostic_component, identifier, strlen(identifier))
        }
    });
}

static void resolver_throw_err_pattern_failed_to_resolve_identifier(Resolver* resolver, Pattern* pattern, const char* identifier)
{
    assert(resolver   != NULL);
    assert(pattern    != NULL);
    assert(identifier != NULL);

    diagnostic_component_push_err(resolver->diagnostic_component, (DiagnosticErr)
    {
        .kind = DIAGNOSTIC_ERR_PATTERN_FAILED_TO_RESOLVE_IDENTIFIER,
        .span = resolver_get_pattern_span(resolver, pattern),
        .err.pattern_failed_to_resolve_identifier =
        {
            .identifier = diagnostic_component_add_identifier
                (resolver->diagnostic_component, identifier, strlen(identifier))
        }
    });
}

static void resolver_throw_err_pattern_failed_to_find_constructor(Resolver* resolver, Pattern* pattern, const char* identifier)
{
    assert(resolver   != NULL);
    assert(pattern    != NULL);
    assert(identifier != NULL);

    diagnostic_component_push_err(resolver->diagnostic_component, (DiagnosticErr)
    {
        .kind = DIAGNOSTIC_ERR_PATTERN_FAILED_TO_FIND_CONSTRUCTOR,
        .span = resolver_get_pattern_span(resolver, pattern),
        .err.pattern_failed_to_find_constructor =
        {
            .identifier = diagnostic_component_add_identifier
                (resolver->diagnostic_component, identifier, strlen(identifier))
        }
    });
}
