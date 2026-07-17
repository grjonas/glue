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

// Attempts to unify two types
// Returns:
//     * 'true'  on successful unification,
//     makes changes to both the left and right types.
//     * 'false' on failure to unify,
//     does not modify the types on either the left or right
bool inferer_unify(Inferer* inferer, Type** left, Type** right)
{
    assert(inferer != NULL);
    assert(left    != NULL);
    assert(right   != NULL);

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
            *type = primary.primary.decl->type;
            if (*type == NULL)
            {
                *type = inferer_create_free_type_var(inferer);
                primary.primary.decl->type = *type;
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

    // struct ExprFn
    // {
    //     int    argc  ;
    //     Expr*  caller;
    //     Expr** argv  ;
    // };
    assert(false);
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
        case EXPR_FN     : assert(false);
        // case EXPR_FN     : return inferer_infer_expr_fn     (inferer, expr->expr.fn     , type);
        default:
            fprintf(stderr, "[%s:%d] Statement parsing: Logical error while parsing statements.\n", __FILE__, __LINE__);
            exit(1);
    }

}

Type* inferer_create_free_type_var(Inferer* inferer)
{
    assert(inferer != NULL);
    assert(false);
}

void inferer_throw_compiler_error(Inferer* inferer, CompileError err)
{
    assert(inferer != NULL);

    CompileError* err_ptr = NULL;
    err_ptr = (CompileError*) arena_push(&inferer->arena, &err, sizeof(CompileError));
    arrput(inferer->errs, err_ptr);
}
