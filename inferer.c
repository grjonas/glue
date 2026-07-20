#include "inferer.h"

static Type private_builtin_type_nil    = (Type) { .kind = TYPE_NIL    };
static Type private_builtin_type_bool   = (Type) { .kind = TYPE_BOOL   };
static Type private_builtin_type_nat    = (Type) { .kind = TYPE_NAT    };
static Type private_builtin_type_int    = (Type) { .kind = TYPE_INT    };
static Type private_builtin_type_real   = (Type) { .kind = TYPE_REAL   };
static Type private_builtin_type_string = (Type) { .kind = TYPE_STRING };

static Type* builtin_type_nil    = &private_builtin_type_nil   ;
static Type* builtin_type_bool   = &private_builtin_type_bool  ;
static Type* builtin_type_nat    = &private_builtin_type_nat   ;
static Type* builtin_type_int    = &private_builtin_type_int   ;
static Type* builtin_type_real   = &private_builtin_type_real  ;
static Type* builtin_type_string = &private_builtin_type_string;

Inferer inferer_init(Resolver* resolver)
{
    assert(resolver != NULL);

    Inferer inferer = (Inferer)
    {
        .txt            = resolver->txt   ,
        .tokens         = resolver->tokens,
        .stmts          = resolver->stmts ,
        .declarations   = resolver->declarations,
        .identifiers    = resolver->identifiers ,
        .arena          = resolver->arena,
        .type_arena     = NULL,
        .type_variables = NULL,
        .errs           = NULL,
    };

    for (int i = 0; i < arrlen(resolver->errs); ++i)
    {
        arrfree(resolver->errs[i]);
    }
    arrfree(resolver->errs);
    arrfree(resolver->context);

    *resolver = (Resolver)
    {
        .txt              = NULL ,
        .tokens           = NULL ,
        .stmts            = NULL ,
        .arena            = NULL ,
        .decl_id          = 0    ,
        .type_variable_id = 0    ,
        .loop_depth       = 0    ,
        .inside_function  = false,
        .context          = NULL ,
        .declarations     = NULL ,
        .identifiers      = NULL ,
        .errs             = NULL ,
    };

    return inferer;
}

void inferer_free(Inferer* inferer)
{
    assert(inferer != NULL);

    free((char*)inferer->txt);
    arrfree(inferer->tokens);
    arena_free(&inferer->arena);
    arena_free(&inferer->type_arena);

    arrfree(inferer->declarations);
    for (int i = 0; i < arrlen(inferer->identifiers); ++i)
    {
        free(inferer->identifiers[i]);
    }
    arrfree(inferer->identifiers );

    for (int i = 0; i < arrlen(inferer->errs); ++i)
    {
        arrfree(inferer->errs[i]);
    }
    arrfree(inferer->errs        );

    for (int i = 0; i < arrlen(inferer->type_variables); ++i)
    {
        arrfree(inferer->type_variables[i]);
    }
    arrfree(inferer->type_variables);

    *inferer = (Inferer)
    {
        .txt            = NULL,
        .tokens         = NULL,
        .stmts          = NULL,
        .declarations   = NULL,
        .identifiers    = NULL,
        .arena          = NULL,
        .type_arena     = NULL,
        .type_variables = NULL,
        .errs           = NULL,
    };
}

// Attempts to unify two types
// Returns:
//     * 'true'  on successful unification,
//     makes changes to both the left and right types.
//     * 'false' on failure to unify,
//     does not modify the types on either the left or right
bool inferer_unify(Inferer* inferer, Type** left_ref, Type** right_ref)
{
    assert(inferer   != NULL);
    assert(left_ref  != NULL);
    assert(right_ref != NULL);

    bool is_sucessful = false;
    Type* left  = *left_ref ;
    Type* right = *right_ref;

    if (right->kind == TYPE_FREE_VAR)
    {
        inferer_bind_variable_to_type(inferer, left, &right);
        goto inferer_unify_end;
    }

    switch (left->kind)
    {
        case TYPE_NIL        : is_sucessful = right->kind == TYPE_NIL        ; break;
        case TYPE_BOOL       : is_sucessful = right->kind == TYPE_BOOL       ; break;
        case TYPE_NAT        : is_sucessful = right->kind == TYPE_NAT        ; break;
        case TYPE_INT        : is_sucessful = right->kind == TYPE_INT        ; break;
        case TYPE_REAL       : is_sucessful = right->kind == TYPE_REAL       ; break;
        case TYPE_STRING     : is_sucessful = right->kind == TYPE_STRING     ; break;

        case TYPE_LIST       : assert(false);
        case TYPE_STRUCT     : assert(false);
        case TYPE_FN         :
            if (right->kind == TYPE_FN)
            {
                is_sucessful = inferer_unify(inferer, &left->type.fn.left, &right->type.fn.left);
                if (!is_sucessful)
                {
                    break;
                }

                inferer_unify(inferer, &left->type.fn.right, &right->type.fn.right);
                if (!is_sucessful)
                {
                    break;
                }
            }
            break;

        case TYPE_FREE_VAR   : inferer_bind_variable_to_type(inferer, left, &right); break;
        case TYPE_BOUNDED_VAR: assert(false);

        case TYPE_NUMERIC    : assert(false);
    }

    inferer_unify_end:

    if (!is_sucessful)
    {
        inferer_throw_compiler_error(inferer, (CompileError)
        {
            .kind   = ERROR_ERROR,
            .line   = -1         ,
            .column = -1         ,
            .length = -1         ,
            .msg    = "Type inference: Failed to unify types.",
        });
    }

    return is_sucessful;
}

// The function 'generalize' abstracts a type over all type variables
// which are free in the type but not free in the given type environment.
bool inferer_generalize(Inferer* inferer, Type* type, Type** scheme)
{
    assert(inferer != NULL);
    assert(type    != NULL);
    assert(scheme  != NULL);
    assert(*scheme == NULL);

    assert(false);
}

bool inferer_constrain_numeric(Inferer* inferer, TypeConstraint* constraint, Type** type)
{
    assert(inferer    != NULL);
    assert(constraint != NULL);
    assert(type       != NULL);
    assert(*type      == NULL);

    bool is_sucessful = false;

    is_sucessful = inferer_unify(inferer, type, &builtin_type_nat);
    if (is_sucessful)
    {
        *constraint = TYPE_CONSTRAINT_NAT;
        *type       = builtin_type_nat   ;
        return true;
    }

    is_sucessful = inferer_unify(inferer, type, &builtin_type_int);
    if (is_sucessful)
    {
        *constraint = TYPE_CONSTRAINT_INT;
        *type       = builtin_type_int   ;
        return true;
    }

    is_sucessful = inferer_unify(inferer, type, &builtin_type_real);
    if (is_sucessful)
    {
        *constraint = TYPE_CONSTRAINT_REAL;
        *type       = builtin_type_real   ;
        return true;
    }

    return false;
}

bool inferer_constrain_equality(Inferer* inferer, TypeConstraint* constraint, Type** type)
{
    assert(inferer    != NULL);
    assert(constraint != NULL);
    assert(type       != NULL);
    assert(*type      == NULL);

    bool is_sucessful = false;

    is_sucessful = inferer_constrain_numeric(inferer, constraint, type);
    if (is_sucessful)
    {
        return true;
    }

    is_sucessful = inferer_unify(inferer, type, &builtin_type_real);
    if (is_sucessful)
    {
        *constraint = TYPE_CONSTRAINT_REAL;
        *type       = builtin_type_real   ;
        return true;
    }

    is_sucessful = inferer_unify(inferer, type, &builtin_type_nil);
    if (is_sucessful)
    {
        *constraint = TYPE_CONSTRAINT_NIL ;
        *type       = builtin_type_nil    ;
        return true;
    }

    is_sucessful = inferer_unify(inferer, type, &builtin_type_bool);
    if (is_sucessful)
    {
        *constraint = TYPE_CONSTRAINT_BOOL;
        *type       = builtin_type_bool   ;
        return true;
    }

    return false;
}

// This function takes a unary argument, and infers it's type.
// Then, it attempts to constrain the type (which can faile, in which case returns false)
// Furthermore, the return type is always a pointer to a builtin type.
bool inferer_infer_expr_and_constrain(Inferer* inferer, Expr* expr, TypeConstraint* constraint, Type** type)
{
    assert(inferer    != NULL);
    assert(expr       != NULL);
    assert(constraint != NULL);
    assert(type       != NULL);
    assert(*type      == NULL);


    bool is_sucessful = true;


    inferer_infer_expr(inferer, expr, type);
    switch (*constraint)
    {
        // These are special cases:
        // Instead of unifying with a specific type, we go down the tree, and attempt to figure out which
        // of a list of types is the correct one.
        case TYPE_CONSTRAINT_NUMERIC    : is_sucessful = inferer_constrain_numeric (inferer, constraint, type); break;
        case TYPE_CONSTRAINT_EQUALITY   : is_sucessful = inferer_constrain_equality(inferer, constraint, type); break;

        case TYPE_CONSTRAINT_NIL        : is_sucessful = inferer_unify(inferer, type, &builtin_type_nil   ); break;
        case TYPE_CONSTRAINT_BOOL       : is_sucessful = inferer_unify(inferer, type, &builtin_type_bool  ); break;
        case TYPE_CONSTRAINT_NAT        : is_sucessful = inferer_unify(inferer, type, &builtin_type_nat   ); break;
        case TYPE_CONSTRAINT_INT        : is_sucessful = inferer_unify(inferer, type, &builtin_type_int   ); break;
        case TYPE_CONSTRAINT_REAL       : is_sucessful = inferer_unify(inferer, type, &builtin_type_real  ); break;
        case TYPE_CONSTRAINT_STRING     : is_sucessful = inferer_unify(inferer, type, &builtin_type_string); break;
        // default:
        //     fprintf(stderr, "[%s:%d] Type inference: Logical error while infering types.\n", __FILE__, __LINE__);
        //     exit(1);
    }

    if (!is_sucessful)
    {
        inferer_throw_compiler_error(inferer, (CompileError)
        {
            .kind   = ERROR_ERROR,
            .line   = -1         ,
            .column = -1         ,
            .length = -1         ,
            .msg    = "Type inference: Failed to unify types.",
        });
        return false;
    }

    return is_sucessful;
}

bool inferer_infer_expr_primary(Inferer* inferer, ExprPrimary primary, Type** type)
{
    assert(inferer != NULL);
    assert(type    != NULL);
    assert(*type   == NULL);

    switch (primary.kind)
    {
        case EXPR_PRIMARY_UNKNOWN   : assert(false);

        // Primitives
        case EXPR_PRIMARY_NIL       : *type = builtin_type_nil   ; return true;
        case EXPR_PRIMARY_BOOLEAN   : *type = builtin_type_bool  ; return true;
        case EXPR_PRIMARY_STRING    : *type = builtin_type_string; return true;
        case EXPR_PRIMARY_NATURAL   : *type = builtin_type_nat   ; return true;
        case EXPR_PRIMARY_INTEGER   : *type = builtin_type_int   ; return true;
        case EXPR_PRIMARY_REAL      : *type = builtin_type_real  ; return true;

        // Derivative
        case EXPR_PRIMARY_LIST      : assert(false);
        case EXPR_PRIMARY_STRUCT    : assert(false);
        case EXPR_PRIMARY_FN        : assert(false);

        // Special

        // Not entirely correct -
        // there are uses where identifier doesn't get resolved (for example, in struct access).
        case EXPR_PRIMARY_IDENTIFIER: assert(false);
        case EXPR_PRIMARY_DECL      :
            *type = inferer_get_decl_var_type(inferer, primary.primary.decl);
            if (*type == NULL)
            {
                *type = inferer_create_free_type_var(inferer);
                inferer_set_decl_var_type(inferer, primary.primary.decl, *type);
            }
            return true;

        default:
            fprintf(stderr, "[%s:%d] Type inference: Logical error while infering types.\n", __FILE__, __LINE__);
            exit(1);
    }
}

bool inferer_infer_expr_unary_logical_operator(Inferer* inferer, ExprUnary unary, Type** type)
{
    assert(inferer     != NULL);
    assert(unary.unary != NULL);
    assert(type        != NULL);
    assert(*type       == NULL);


    bool is_sucessful = false;
    TypeConstraint unary_constraint = TYPE_CONSTRAINT_BOOL;


    is_sucessful = inferer_infer_expr_and_constrain
        (inferer, unary.unary, &unary_constraint, type);
    if (!is_sucessful)
        return false;

    return true;
}

bool inferer_infer_expr_unary_arithmetic_operator(Inferer* inferer, ExprUnary unary, Type** type)
{
    assert(inferer     != NULL);
    assert(unary.unary != NULL);
    assert(type        != NULL);
    assert(*type       == NULL);


    bool is_sucessful = false;
    TypeConstraint unary_constraint = TYPE_CONSTRAINT_NUMERIC;


    is_sucessful = inferer_infer_expr_and_constrain
        (inferer, unary.unary, &unary_constraint, type);
    if (!is_sucessful)
        return false;

    return true;
}

bool inferer_infer_expr_unary(Inferer* inferer, ExprUnary unary, Type** type)
{
    assert(inferer != NULL);
    assert(type        != NULL);
    assert(*type       == NULL);

    switch (unary.kind)
    {
        case EXPR_UNARY_UNKNOWN: assert(false);
        case EXPR_UNARY_NOT    : return inferer_infer_expr_unary_logical_operator   (inferer, unary, type);
        case EXPR_UNARY_NEGATE : return inferer_infer_expr_unary_arithmetic_operator(inferer, unary, type);
        default:
            fprintf(stderr, "[%s:%d] Type inference: Logical error while infering types.\n", __FILE__, __LINE__);
            exit(1);
    }
}

bool inferer_infer_expr_binary_arithmetic_operator(Inferer* inferer, ExprBinary binary, Type** type)
{
    assert(inferer != NULL);
    assert(binary.left  != NULL);
    assert(binary.right != NULL);
    assert(type         != NULL);
    assert(*type        == NULL);


    bool is_sucessful = false;
    Type* left_return_type  = NULL;
    Type* right_return_type = NULL;
    TypeConstraint left_constraint  = TYPE_CONSTRAINT_NUMERIC;
    TypeConstraint right_constraint = TYPE_CONSTRAINT_NUMERIC;


    is_sucessful = inferer_infer_expr_and_constrain(inferer, binary.left , &left_constraint , &left_return_type );
    if (!is_sucessful)
        return false;

    is_sucessful = inferer_infer_expr_and_constrain(inferer, binary.right, &right_constraint, &right_return_type);
    if (!is_sucessful)
        return false;

    // Now we need to make sure that types on both sides are the same type.
    // This is to make sure that operators like (for example) 'Int -> Real -> Real' are not possible.
    if (left_constraint != right_constraint)
    {
        inferer_throw_compiler_error(inferer, (CompileError)
        {
            .kind   = ERROR_ERROR,
            .line   = -1         ,
            .column = -1         ,
            .length = -1         ,
            .msg    = "Type inference: Failed to unify both sides of binary arithmetic operation.",
        });
        return false;
    }

    *type = left_return_type;
    return true;
}

bool inferer_infer_expr_binary_logical_operator(Inferer* inferer, ExprBinary binary, Type** type)
{
    assert(inferer != NULL);
    assert(binary.left  != NULL);
    assert(binary.right != NULL);
    assert(type         != NULL);
    assert(*type        == NULL);


    bool is_sucessful = false;
    Type* left_return_type  = NULL;
    Type* right_return_type = NULL;
    TypeConstraint left_constraint  = TYPE_CONSTRAINT_BOOL;
    TypeConstraint right_constraint = TYPE_CONSTRAINT_BOOL;

    is_sucessful = inferer_infer_expr_and_constrain(inferer, binary.left , &left_constraint , &left_return_type );
    if (!is_sucessful)
        return false;

    is_sucessful = inferer_infer_expr_and_constrain(inferer, binary.right, &right_constraint, &right_return_type);
    if (!is_sucessful)
        return false;

    *type = builtin_type_bool;
    return true;
}

bool inferer_infer_expr_binary_equality_operator(Inferer* inferer, ExprBinary binary, Type** type)
{
    assert(inferer != NULL);
    assert(binary.left  != NULL);
    assert(binary.right != NULL);
    assert(type         != NULL);
    assert(*type        == NULL);


    bool is_sucessful = false;
    Type* left_return_type  = NULL;
    Type* right_return_type = NULL;
    TypeConstraint left_constraint  = TYPE_CONSTRAINT_EQUALITY;
    TypeConstraint right_constraint = TYPE_CONSTRAINT_EQUALITY;

    assert(false);
    is_sucessful = inferer_infer_expr_and_constrain(inferer, binary.left , &left_constraint , &left_return_type );
    if (!is_sucessful)
        return false;

    is_sucessful = inferer_infer_expr_and_constrain(inferer, binary.right, &right_constraint, &right_return_type );
    if (!is_sucessful)
        return false;

    *type = builtin_type_bool;
    return true;
}

// TODO: Rethink think how to resolve struct access operator.
bool inferer_infer_expr_binary(Inferer* inferer, ExprBinary binary, Type** type)
{
    assert(inferer != NULL);
    assert(type    != NULL);
    assert(*type   == NULL);


    switch (binary.kind)
    {
        case EXPR_BINARY_UNKNOWN      : assert(false);        // Arithmetic operators
        case EXPR_BINARY_ADD          : return inferer_infer_expr_binary_arithmetic_operator(inferer, binary, type);
        case EXPR_BINARY_SUBTRACT     : return inferer_infer_expr_binary_arithmetic_operator(inferer, binary, type);
        case EXPR_BINARY_MULTIPLY     : return inferer_infer_expr_binary_arithmetic_operator(inferer, binary, type);
        case EXPR_BINARY_DIVIDE       : return inferer_infer_expr_binary_arithmetic_operator(inferer, binary, type);
        case EXPR_BINARY_MODULO       : return inferer_infer_expr_binary_arithmetic_operator(inferer, binary, type);

        // Logical operators
        case EXPR_BINARY_AND          : return inferer_infer_expr_binary_logical_operator   (inferer, binary, type);
        case EXPR_BINARY_OR           : return inferer_infer_expr_binary_logical_operator   (inferer, binary, type);

        // Equality
        case EXPR_BINARY_EQUAL        : return inferer_infer_expr_binary_equality_operator  (inferer, binary, type);
        case EXPR_BINARY_NOT_EQUAL    : return inferer_infer_expr_binary_equality_operator  (inferer, binary, type);

        // Comparison
        // for now, comparisons are resolved similiarly to arithmetic expressions.
        case EXPR_BINARY_LESS_EQUAL   : return inferer_infer_expr_binary_arithmetic_operator(inferer, binary, type);
        case EXPR_BINARY_LESS         : return inferer_infer_expr_binary_arithmetic_operator(inferer, binary, type);
        case EXPR_BINARY_GREATER_EQUAL: return inferer_infer_expr_binary_arithmetic_operator(inferer, binary, type);
        case EXPR_BINARY_GREATER      : return inferer_infer_expr_binary_arithmetic_operator(inferer, binary, type); // TODO: Implement the rest of these binary operators.
        case EXPR_BINARY_CHAIN        : assert(false); // Shouldn't be encountered ideally
        case EXPR_BINARY_ACCESS       : assert(false);
        case EXPR_BINARY_ASSIGN       : assert(false);
        case EXPR_BINARY_INDEX        : assert(false);

        default:
            fprintf(stderr, "[%s:%d] Type inference: Logical error while infering types.\n", __FILE__, __LINE__);
            exit(1);
    }
}

bool inferer_infer_expr_fn(Inferer* inferer, ExprFn fn, Type** type)
{
    assert(inferer != NULL);
    assert(type    != NULL);
    assert(*type   == NULL);

    bool is_sucessful   = false;
    Type* caller_type   = NULL ;
    Type* fn_type       = NULL ;
    Type* curr_fn_ptr   = NULL ;
    Type* curr_arg_type = NULL ;

    assert(false);

    is_sucessful = inferer_infer_expr(inferer, fn.caller, &caller_type);
    if (!is_sucessful)
    {
        return false;
    }

    fn_type = inferer_create_free_function_type(inferer, fn.argc);
    curr_fn_ptr = fn_type;

    for (int i = 0; i < fn.argc; ++i)
    {
        is_sucessful = inferer_infer_expr(inferer, fn.argv[i], &curr_arg_type);
        if (!is_sucessful)
        {
            return false;
        }

        is_sucessful = inferer_unify(inferer, &curr_fn_ptr->type.fn.left, &curr_arg_type);
        if (!is_sucessful)
        {
            return false;
        }

        curr_fn_ptr = fn_type->type.fn.right;
    }

    return true;
}

bool inferer_infer_expr(Inferer* inferer, Expr* expr, Type** type)
{
    assert(inferer != NULL);
    assert(expr    != NULL);
    assert(type    != NULL);
    assert(*type   == NULL);

    switch (expr->kind)
    {
        case EXPR_PRIMARY: return inferer_infer_expr_primary(inferer, expr->expr.primary, type);
        case EXPR_UNARY  : return inferer_infer_expr_unary  (inferer, expr->expr.unary  , type);
        case EXPR_BINARY : return inferer_infer_expr_binary (inferer, expr->expr.binary , type);
        case EXPR_FN     : return inferer_infer_expr_fn     (inferer, expr->expr.fn     , type);
        default:
            fprintf(stderr, "[%s:%d] Type inference: Logical error while parsing expression.\n", __FILE__, __LINE__);
            exit(1);
    }
}

bool inferer_infer_type_expr_variable(Inferer* inferer, TypeExprVariable variable, Type** type)
{
    assert(inferer   != NULL);
    assert(type      != NULL);
    assert(*type     == NULL);

    assert(false);

    // TypeExprVariable works sort of like either a function if the arity is above 0,
    // OR variable, if the arity is 0.
    assert(false);
    if (variable.argc > 0)
    {
        return true;
    }
    else
    {
        return true;
    }
}

bool inferer_infer_type_expr(Inferer* inferer, TypeExpr* type_expr, Type** type)
{
    assert(inferer   != NULL);
    assert(type_expr != NULL);
    assert(type      != NULL);
    assert(*type     == NULL);

    switch(type_expr->kind)
    {
        /*
            fn map(fun : a -> b, ls_a : List(a))
            do
                # The 'b' in 'List(b)' should be the same as the one in 'a -> b'.
                let ls_b : List(b) = fun(ls_a)
                return ls_b
            end
         */
        case TYPE_EXPR_VARIABLE  : return inferer_infer_type_expr_variable(inferer, type_expr->type_expr.variable, type);
        case TYPE_EXPR_IDENTIFIER: assert(false); // Shouldn't be encountered at this stage.

        case TYPE_EXPR_NIL       : return builtin_type_nil   ; 
        case TYPE_EXPR_BOOL      : return builtin_type_bool  ; 
        case TYPE_EXPR_NAT       : return builtin_type_nat   ; 
        case TYPE_EXPR_INT       : return builtin_type_int   ; 
        case TYPE_EXPR_REAL      : return builtin_type_real  ; 
        case TYPE_EXPR_STRING    : return builtin_type_string; 

        case TYPE_EXPR_LIST      : assert(false); 
        case TYPE_EXPR_STRUCT    : assert(false); 
        case TYPE_EXPR_FN        : assert(false);

        case TYPE_EXPR_INSTANCE  : assert(false);
    }

    fprintf(stderr, "[%s:%d] Type inference: Logical error while parsing type expression.\n", __FILE__, __LINE__);
    exit(1);
}

bool inferer_infer_stmt_block(Inferer* inferer, StmtBlock block)
{
    assert(inferer    != NULL);
    assert(block.body != NULL);

    for (int i = 0; i < block.size; ++i)
    {
        Stmt* inner_stmt = block.body[i];
        bool is_sucessful = inferer_infer_stmt(inferer, inner_stmt);
        if (!is_sucessful)
        {
            return false;
        }
    }

    return true;
}

bool inferer_infer_stmt_while(Inferer* inferer, StmtWhile whilee)
{
    assert(inferer    != NULL);

    bool is_sucessful = false;
    Type* type = NULL;

    is_sucessful = inferer_infer_expr(inferer, whilee.condition, &type);
    if (!is_sucessful)
    {
        return false;
    }

    is_sucessful = inferer_infer_stmt(inferer, whilee.body);
    if (!is_sucessful)
    {
        return false;
    }

    return true;
}

bool inferer_infer_stmt_if(Inferer* inferer, StmtIf iff)
{
    assert(inferer    != NULL);

    bool is_sucessful = false;
    Type* type = NULL;

    is_sucessful = inferer_infer_expr(inferer, iff.condition, &type);
    if (!is_sucessful)
    {
        return false;
    }

    is_sucessful = inferer_infer_stmt(inferer, iff.body);
    if (!is_sucessful)
    {
        return false;
    }

    return true;
}

bool inferer_infer_stmt_return(Inferer* inferer, StmtReturn returnn)
{
    assert(inferer    != NULL);

    Type* type = NULL;

    if (returnn.expr != NULL && !inferer_infer_expr(inferer, returnn.expr, &type))
    {
        return false;
    }

    return true;
}

bool inferer_infer_stmt_let(Inferer* inferer, StmtLet let)
{
    assert(inferer != NULL);

    bool is_sucessful = false;
    Type* expr_type = NULL;
    Type* type      = NULL;
    Type* scheme    = NULL;

    is_sucessful = inferer_infer_expr(inferer, let.expr, &expr_type);
    if (!is_sucessful)
    {
        return false;
    }

    if (let.type != NULL)
    {
        is_sucessful = inferer_infer_type_expr(inferer, let.type, &type);
        if (!is_sucessful)
        {
            return false;
        }

        inferer_set_decl_var_type(inferer, let.decl, type);
    }

    is_sucessful = inferer_generalize(inferer, inferer_get_decl_var_type(inferer, let.decl), &scheme);
    if (!is_sucessful)
    {
        return false;
    }

    assert(scheme->kind == TYPE_SCHEME);
    inferer_set_decl_var_type(inferer, let.decl, scheme);
    return true;
}

bool inferer_infer_stmt_let(Inferer* inferer, StmtLet let)
{
    assert(inferer != NULL);

    bool is_sucessful = false;

    Type* expr_type       = NULL;
    Type* annotation_type = NULL;
    Type* final_type      = NULL;
    Type* scheme          = NULL;

    is_sucessful = inferer_infer_expr(inferer, let.expr, &expr_type);
    if (!is_sucessful)
    {
        return false;
    }

    if (let.type != NULL)
    {
        is_sucessful = inferer_infer_type_expr(inferer, let.type, &annotation_type);
        if (!is_sucessful)
        {
            return false;
        }

        is_sucessful = inferer_unify(inferer, expr_type, annotation_type);
        if (!is_sucessful)
        {
            return false;
        }

        final_type = annotation_type;
    }
    else
    {
        final_type = expr_type;
    }

    is_sucessful = inferer_generalize(inferer, final_type, &scheme);
    if (!is_sucessful)
    {
        return false;
    }

    assert(scheme != NULL);
    assert(scheme->kind == TYPE_SCHEME);

    inferer_set_decl_var_type(inferer, let.decl, scheme);

    return true;
}

// bool inferer_infer_stmt_fn(Inferer* inferer, StmtFn fn)
// {
//     assert(inferer != NULL);
// 
//     bool  is_successful = false;
//     Type* return_type   = NULL;
//     Type* function_type = NULL;
//     Type* scheme        = NULL;
// 
//     if (fn.return_type != NULL)
//     {
//         is_successful = inferer_infer_type_expr(inferer, fn.return_type, &return_type);
//         if (!is_successful)
//         {
//             return false;
//         }
//     }
// 
//     is_successful = inferer_infer_stmt(inferer, fn.body);
//     if (!is_successful)
//     {
//         return false;
//     }
// 
//     function_type = inferer_get_decl_var_type(inferer, fn.decl);
// 
//     if (return_type != NULL)
//     {
//         Type* body_return_type = inferer_get_function_return_type(function_type);
// 
//         is_successful = inferer_get_most_general_unifier(inferer, body_return_type, return_type);
//         if (!is_successful)
//         {
//             return false;
//         }
//     }
// 
//     is_successful = inferer_generalize(inferer, function_type, &scheme);
//     if (!is_successful)
//     {
//         return false;
//     }
// 
//     assert(scheme != NULL);
//     assert(scheme->kind == TYPE_SCHEME);
// 
//     inferer_set_decl_var_type(inferer, fn.decl, scheme);
// 
//     return true;
// }

bool inferer_infer_stmt(Inferer* inferer, Stmt* stmt)
{
    assert(inferer != NULL);
    assert(stmt    != NULL);
    // assert(type    != NULL);
    // assert(*type   == NULL);

    Type* type = NULL;

    switch (stmt->kind)
    {

        case STMT_BLOCK   : return inferer_infer_stmt_block (inferer, stmt->stmt.block  );
        case STMT_LET     : return inferer_infer_stmt_let   (inferer, stmt->stmt.let    ); 
        case STMT_EXPR    : return inferer_infer_expr       (inferer, stmt->stmt.expr, &type);
        case STMT_IF      : return inferer_infer_stmt_if    (inferer, stmt->stmt.iff    );
        case STMT_WHILE   : return inferer_infer_stmt_while (inferer, stmt->stmt.whilee );
        case STMT_BREAK   : return true; 
        case STMT_CONTINUE: return true;
        case STMT_FN      : return inferer_infer_stmt_fn    (inferer, stmt->stmt.fn     );
        case STMT_RETURN  : return inferer_infer_stmt_return(inferer, stmt->stmt.returnn);
        case STMT_ALIAS   : assert(false);
        case STMT_TYPE    : assert(false);
    }

    fprintf(stderr, "[%s:%d] Type inference: Logical error while parsing statement.\n", __FILE__, __LINE__);
    exit(1);
}

Type* inferer_create_free_type_var(Inferer* inferer)
{
    assert(inferer != NULL);

    Type type_mem;
    Type* type = NULL;

    type_mem = (Type)
    {
        .kind = TYPE_FREE_VAR,
        .type.free_var.type = NULL,
    };

    type = (Type*) arena_push(&inferer->type_arena, &type_mem, sizeof(Type));
    arrput(inferer->type_variables, type);

    return type;
}

Type* inferer_create_free_function_type(Inferer* inferer, int arity)
{
    assert(inferer != NULL);

    Type type_mem;
    Type* type      = NULL;
    Type* curr_type = NULL;

    type_mem = (Type)
    {
        .kind = TYPE_FN,
        .type.fn = (TypeFn)
        {
            .left  = arity == 0 ? NULL : inferer_create_free_type_var(inferer),
            .right = inferer_create_free_type_var(inferer),
        }
    };

    type = (Type*) arena_push(&inferer->type_arena, &type_mem, sizeof(Type));
    curr_type = type;

    for (int i = 1; i < arity; ++i)
    {
        type_mem = (Type)
        {
            .kind = TYPE_FN,
            .type.fn = (TypeFn)
            {
                .left  = inferer_create_free_type_var(inferer),
                .right = curr_type->type.fn.right,
            }
        };

        curr_type->type.fn.right = (Type*) arena_push(&inferer->type_arena, &type_mem, sizeof(Type));
    }

    return type;
}

Type* inferer_get_decl_var_type(Inferer* inferer, Decl* decl)
{
    assert(inferer != NULL);
    assert(decl    != NULL);
    assert(decl->kind == DECL_VAR);

    return decl->decl.var.type;
}

void inferer_set_decl_var_type(Inferer* inferer, Decl* decl, Type* type)
{
    assert(inferer != NULL);
    assert(decl    != NULL);
    assert(decl->kind == DECL_VAR);
    // type can be NULL ?

    decl->decl.var.type = type;
}

void inferer_bind_variable_to_type(Inferer* inferer, Type* var, Type** type)
{
    assert(inferer   != NULL         );
    assert(var       != NULL         );
    assert(type      != NULL         );
    assert(var->kind == TYPE_FREE_VAR);

    if (var->type.free_var.type != NULL)
    {
        fprintf(stderr, "[%s:%d] Type inference: Cannot bind type to variable that already has a type bound to it.\n", __FILE__, __LINE__);
        exit(1);
    }

    var->type.free_var.type = *type;
}

void inferer_throw_compiler_error(Inferer* inferer, CompileError err)
{
    assert(inferer != NULL);

    CompileError* err_ptr = NULL;
    err_ptr = (CompileError*) arena_push(&inferer->arena, &err, sizeof(CompileError));
    arrput(inferer->errs, err_ptr);
}
