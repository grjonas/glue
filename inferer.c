#include "inferer.h"

bool inferer_infer_expr_primary(Inferer* inferer, ExprPrimary primary, Type** type)
{
    bool  is_sucessful = false;

    switch (primary.kind)
    {
        case EXPR_PRIMARY_UNKNOWN   : assert(false);

        // Primitives
        case EXPR_PRIMARY_NIL       : *type = BUILTIN_TYPE_NIL   ; return true;
        case EXPR_PRIMARY_BOOLEAN   : *type = BUILTIN_TYPE_BOOL  ; return true;
        case EXPR_PRIMARY_STRING    : *type = BUILTIN_TYPE_STRING; return true;
        case EXPR_PRIMARY_NATURAL   : *type = BUILTIN_TYPE_NAT   ; return true;
        case EXPR_PRIMARY_INTEGER   : *type = BUILTIN_TYPE_INT   ; return true;
        case EXPR_PRIMARY_REAL      : *type = BUILTIN_TYPE_REAL  ; return true;

        // Derivative
        case EXPR_PRIMARY_LIST      : assert(false);
        case EXPR_PRIMARY_STRUCT    : assert(false);
        case EXPR_PRIMARY_FN        : assert(false);

        // Special

        // Not entirely correct -
        // there are uses where identifier doesn't get resolved (for example, in struct access).
        case EXPR_PRIMARY_IDENTIFIER: assert(false);
        case EXPR_PRIMARY_DECL      :
            *type = inferer_lookup_decl_type(inferer, primary.primary.decl);
            if (type == NULL)
            {
                *type = inferer_create_free_type_var(inferer);
                inferer_decl_set_type(inferer, primary.primary.decl, *type);
            }
            return true;

        default:
            fprintf(stderr, "[%s:%d] Type inference: Logical error while infering types.\n", __FILE__, __LINE__);
            exit(1);
    }
}

// This function takes a unary argument, and infers it's type.
// Then, it attempts to unify with the 'unary_type' type.
// NOTE: Only works with basic types, doesn't work with derived ones (such as list, function, etc.)
bool inferer_infer_and_unify_with_type_of_type_kind(Inferer* inferer, Expr* expr, TypeKind** expr_type, Type** type)
{
    assert(expr      != NULL);
    assert(expr_type != NULL);
    assert_generic_operator_type_is_valid(*expr_type);


    bool is_sucessful = true;


    inferer_infer_expr(inferer, expr, type);
    switch (*expr_type)
    {
        // This is a bit of a unique case:
        // Instead of unifying with a specific type, we go down the tree, and attempt to figure out which
        // Numeric type we have (if possible);
        case TYPE_NUMERIC    :
            is_sucessful = inferer_unify(inferer, type, &BUILTIN_TYPE_NAT);
            if (is_sucessful)
            {
                *expr_type = BUILTIN_TYPE_NAT;
                break;
            }

            is_sucessful = inferer_unify(inferer, type, &BUILTIN_TYPE_INT);
            if (is_sucessful)
            {
                *expr_type = BUILTIN_TYPE_INT;
                break;
            }

            is_sucessful = inferer_unify(inferer, type, &BUILTIN_TYPE_REAL);
            if (is_sucessful)
            {
                *expr_type = BUILTIN_TYPE_REAL;
                break;
            }

            is_sucessful = false;;

        case TYPE_NIL        : is_sucessful = inferer_unify(inferer, type, &BUILTIN_TYPE_NIL   ); break;
        case TYPE_BOOL       : is_sucessful = inferer_unify(inferer, type, &BUILTIN_TYPE_NIL   ); break;
        case TYPE_NAT        : is_sucessful = inferer_unify(inferer, type, &BUILTIN_TYPE_NAT   ); break;
        case TYPE_INT        : is_sucessful = inferer_unify(inferer, type, &BUILTIN_TYPE_INT   ); break;
        case TYPE_REAL       : is_sucessful = inferer_unify(inferer, type, &BUILTIN_TYPE_REAL  ); break;
        case TYPE_STRING     : is_sucessful = inferer_unify(inferer, type, &BUILTIN_TYPE_STRING); break;
        default:
            fprintf(stderr, "[%s:%d] Type inference: Logical error while infering types.\n", __FILE__, __LINE__);
            exit(1);
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

bool inferer_infer_and_unify_expr_unary_generic(Inferer* inferer, Expr* unary, TypeKind* unary_type, Type** type)
{
    assert(unary != NULL);
    assert(unary_kind != NULL);
    assert_generic_operator_type_is_valid(*unary_type);

    return inferer_infer_and_unify_with_type_of_type_kind(inferer, unary, unary_type, type)
}

bool inferer_infer_and_unify_expr_binary_generic(Inferer* inferer, Expr* left, TypeKind* left_type, Expr* right, TypeKind* right_type, Type** type)
{
    assert(left  != NULL);
    assert(right != NULL);
    assert(left_type  != NULL);
    assert(right_type != NULL);
    assert_generic_operator_type_is_valid(*left_type );
    assert_generic_operator_type_is_valid(*right_type);


    bool is_sucessful = false;
    Type* left_return_type  = NULL;
    Type* right_return_type = NULL;


    is_sucessful = inferer_infer_and_unify_with_type_of_type_kind(inferer, left , left_type , &left_return_type );
    if (!is_sucessful)
        return false;

    is_sucessful = inferer_infer_and_unify_with_type_of_type_kind(inferer, right, right_type, &right_return_type );
    if (!is_sucessful)
        return false;
}

bool inferer_infer_expr_unary_logical_operator(Inferer* inferer, ExprUnary unary, Type** type)
{
    bool is_sucessful = false;
    TypeKind return_kind = TYPE_BOOL;

    is_sucessful = inferer_infer_expr_unary_generic_operator
        (inferer, unary.unary, return_kind, type);
    if (!is_sucessful)
        return false;
    *type = &BUILTIN_TYPE_BOOL; 

    return true;
}

bool inferer_infer_expr_unary_arithmetic_operator(Inferer* inferer, ExprUnary unary, Type** type)
{
    bool is_sucessful = false;
    TypeKind return_kind = TYPE_NUMERIC;

    is_sucessful = inferer_infer_expr_unary_generic_operator
        (inferer, unary.unary, return_kind, type);
    if (!is_sucessful)
        return false;
    *type = type_kind_to_type_ptr(*return_kind);

    return true;
}

bool inferer_infer_expr_unary(Inferer* inferer, ExprUnary unary, Type** type)
{
    assert(inferer != NULL);

    bool is_sucessful = false;
    TypeKind unary_kind;

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

// TODO: Rethink think how to resolve struct access operator.
bool inferer_infer_expr_binary(Inferer* inferer, ExprBinary binary, Type** type)
{
    assert(false);
    switch (binary.kind)
    {
        case EXPR_BINARY_UNKNOWN      : assert(false);
        case EXPR_BINARY_ADD          :
            return NULL;
        case EXPR_BINARY_SUBTRACT     :
            return NULL;
        case EXPR_BINARY_MULTIPLY     :
            return NULL;
        case EXPR_BINARY_DIVIDE       :
            return NULL;
        case EXPR_BINARY_MODULO       :
            return NULL;
        case EXPR_BINARY_AND          :
            return NULL;
        case EXPR_BINARY_OR           :
            return NULL;
        case EXPR_BINARY_EQUAL        :
            return NULL;
        case EXPR_BINARY_NOT_EQUAL    :
            return NULL;
        case EXPR_BINARY_LESS_EQUAL   :
            return NULL;
        case EXPR_BINARY_LESS         :
            return NULL;
        case EXPR_BINARY_GREATER_EQUAL:
            return NULL;
        case EXPR_BINARY_GREATER      :
            return NULL;
        case EXPR_BINARY_CHAIN        :
            return NULL;
        case EXPR_BINARY_ACCESS       :
            return NULL;
        case EXPR_BINARY_ASSIGN       :
            return NULL;
        case EXPR_BINARY_INDEX        :
            return NULL;

        default:
            fprintf(stderr, "[%s:%d] Type inference: Logical error while infering types.\n", __FILE__, __LINE__);
            exit(1);
    }
}

bool inferer_infer_expr(Inferer* inferer, Expr* expr, Type** type)
{
    assert(inferer != NULL);
    assert(expr    != NULL);
    assert(type    != NULL);

    switch (expr->kind)
    {
        case EXPR_PRIMARY: return inferer_infer_expr_primary(inferer, expr->expr.primary, type);
        case EXPR_UNARY  : return inferer_infer_expr_unary  (inferer, expr->expr.unary  , type);
        case EXPR_BINARY : return inferer_infer_expr_binary (inferer, expr->expr.binary , type);
        case EXPR_FN     : return inferer_infer_expr_fn     (inferer, expr->expr.fn     , type);
        default:
            fprintf(stderr, "[%s:%d] Statement parsing: Logical error while parsing statements.\n", __FILE__, __LINE__);
            exit(1);
    }

}

void assert_generic_operator_type_is_valid(TypeKind type)
{
    bool result = false;

    switch (type)
    {
        case TYPE_NIL        : result = true ; break;
        case TYPE_BOOL       : result = true ; break;
        case TYPE_NUMERIC    : result = true ; break;
        case TYPE_INT        : result = true ; break;
        case TYPE_REAL       : result = true ; break;
        case TYPE_STRING     : result = true ; break;
        case TYPE_LIST       : result = false; break;
        case TYPE_STRUCT     : result = false; break;
        case TYPE_FN         : result = false; break;
        case TYPE_FREE_VAR   : result = false; break;
        case TYPE_BOUNDED_VAR: result = false; break;
    }

    assert(result);
}

void inferer_throw_compiler_error(Inferer* inferer, CompileError err)
{
    CompileError* err_ptr = NULL;
    err_ptr = (CompileError*) arena_push(&inferer->arena, &err, sizeof(CompileError));
    arrput(inferer->errs, err_ptr);
}
