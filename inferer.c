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
            fprintf(stderr, "[%s:%d] Statement parsing: Logical error while parsing statements.\n", __FILE__, __LINE__);
            exit(1);
    }
}

bool inferer_infer_expr_unary(Inferer* inferer, ExprUnary unary, Type** type)
{
    assert(false);
    switch (unary.kind)
    {
        case EXPR_UNARY_UNKNOWN:
            assert(false);

        case EXPR_UNARY_NOT    :
            return NULL;

        case EXPR_UNARY_NEGATE :
            return NULL;

        default:
            fprintf(stderr, "[%s:%d] Statement parsing: Logical error while parsing statements.\n", __FILE__, __LINE__);
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

void inferer_throw_compiler_error(Inferer* inferer, CompileError err)
{
    CompileError* err_ptr = NULL;
    err_ptr = (CompileError*) arena_push(&inferer->arena, &err, sizeof(CompileError));
    arrput(inferer->errs, err_ptr);
}
