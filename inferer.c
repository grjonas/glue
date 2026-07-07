#include "inferer.h"

Inferer inferer_init(Resolver* resolver)
{
    arrfree(resolver->context     );
    arrfree(resolver->errs        );

    *resolver = (Resolver)
    {
        .txt              = NULL                    ,
        .tokens           = NULL                    ,
        .stmts            = NULL                    ,
        .arena            = resolver->arena         ,
        .decl_id          = 0                       ,
        .type_variable_id = 0                       ,
        .loop_depth       = 0                       ,
        .inside_function  = false                   ,
        .context          = NULL                    ,
        .declarations     = NULL                    ,
        .identifiers      = NULL                    ,
        .errs             = NULL                    ,
    };

    return (Inferer)
    {
        .txt          = resolver->txt         ,
        .tokens       = resolver->tokens      ,
        .stmts        = resolver->stmts       ,
        .declarations = resolver->declarations,
        .identifiers  = resolver->identifiers ,
        .arena        = resolver->arena       ,
        .errs         = NULL                  ,
    };
}

void    inferer_free(Inferer* inferer  )
{
    free((char*) inferer->txt);
    arrfree(inferer->tokens);
    arrfree(inferer->stmts);
    arrfree(inferer->declarations);
    arrfree(inferer->identifiers);
    arena_free(&inferer->arena);
    arrfree(inferer->errs);

    *inferer = (Inferer)
    {
        .txt          = NULL,
        .tokens       = NULL,
        .stmts        = NULL,
        .declarations = NULL,
        .identifiers  = NULL,
        .errs         = NULL,
    };
}

// Type* inferer_infer_expr(Inferer* infer, TypeEnv* env, Expr* expr)
// {
//     assert(expr != NULL);
// 
//     Type* type = NULL;
// 
//     switch (expr->kind)
//     {
//         case EXPR_PRIMARY:
//             return inferer_infer_expr_primary(inferer, env, expr->expr.primary);
//             break;
// 
//         case EXPR_UNARY  :
//             break;
// 
//         case EXPR_BINARY :
//             break;
// 
//         case EXPR_FN     :
//             break;
//     };
// }
// 
// Type* inferer_infer_expr_primary(Inferer* inferer, TypeEnv* env, ExprPrimary primary)
// {
//     Type*   type   = NULL;
//     Scheme* scheme = NULL;
// 
//     TypeStructField*  type_field  = NULL;
//     TypeStructField** type_struct = NULL;
// 
//     switch (primary.kind)
//     {
//         case EXPR_PRIMARY_UNKNOWN   :
//             fprintf(stderr, "[%s:%d] Type inference: Detected unknown primary expression during inference.\n", __FILE__, __LINE__);
//             exit(1);
// 
//         case EXPR_PRIMARY_NIL       :
//             type = inferer_get_type_nil(inferer);
//             break;
// 
//         case EXPR_PRIMARY_BOOLEAN   :
//             type = inferer_get_type_bool(inferer);
//             break;
// 
//         case EXPR_PRIMARY_STRING    :
//             type = inferer_get_type_string(inferer);
//             break;
// 
//         case EXPR_PRIMARY_NATURAL   :
//             type = inferer_get_type_natural(inferer);
//             break;
// 
//         case EXPR_PRIMARY_INTEGER   :
//             type = inferer_get_type_integer(inferer);
//             break;
// 
//         case EXPR_PRIMARY_REAL      :
//             type = inferer_get_type_real(inferer);
//             break;
// 
//         case EXPR_PRIMARY_STRUCT    :
//             // for (int i = 0; i < primary.primary.structt.argc; ++i)
//             // {
//             //     ExprPrimaryStructField* field = primary.primary.structt.argv[i];
//             //     if (field.value != NULL)
//             //     {
//             //         type = inferer_infer_expr(inferer, env, field.value);
//             //     }
//             // }
//             fprintf(stderr, "[%s:%d] Type inference: Not implemented.\n", __FILE__, __LINE__);
//             exit(1);
// 
//         case EXPR_PRIMARY_FN        :
//             fprintf(stderr, "[%s:%d] Type inference: Not implemented.\n", __FILE__, __LINE__);
//             exit(1);
// 
//         case EXPR_PRIMARY_IDENTIFIER:
//             fprintf(stderr, "[%s:%d] Type inference: Somehow found identifier expression.\n", __FILE__, __LINE__);
//             exit(1);
// 
//         case EXPR_PRIMARY_DECL      :
//             scheme = inferer_type_env_lookup(inferer, env, primary.primary.decl);
//             if (scheme == NULL)
//             {
//                 assert("Error! Replace Later with throw error." == NULL);
//                 return NULL;
//             }
//             type = inferer_scheme_instantiate(inferer, scheme);
//             break;
//     }
// 
//     return type;
// }
// 
// Subst* inferer_unify(Inferer* inferer, Type* left, Type* right)
// {
//     assert(left  != NULL);
//     assert(right != NULL);
// 
//     Subst* subst = NULL;
// 
//     switch (left->kind)
//     {
//     }
// }

// get_all_free_type_variables : a -> int** free_var_id;
// apply_substition : Subst -> a -> a -- only replaces free type variables
// remove : TypeEnv -> int term_id -> TypeEnv
// generalize : TypeEnv -> Type -> Scheme -- abstracts a type over all variables which are free in the the type but not free in the type env
// instantiate : Scheme -> Type

// w : TypeEnv X Expr -> Subst X Type
// mgu : Type -> Type -> TI Subst

// Allocated using stb, not to an arena. (NULL on failure)
int* inferer_get_free_type_vars_from_type(Inferer* inferer, Type* type)
{
}

void inferer_apply_substitution_to_type(Inferer* inferer, Subst* subst, Type* type)
{
}

bool inferer_type_env_remove_term(Inferer* inferer, TypeEnv* env, int term_id)
{
}

bool inferer_generalize_type(Inferer* inferer, TypeEnv* env, Type* type, Scheme* subst_ref)
{
}

bool inferer_instantiate_scheme(Inferer* inferer, Scheme* scheme, Type** type_ref)
{
}

// always returns false
bool inferer_throw_failed_to_unify_error(Inferer* inferer)
{
    inferer_throw_compiler_error(inferer, (CompileError)
    {
        .kind   = ERROR_ERROR,
        .line   = -1         ,
        .column = -1         ,
        .length = -1         ,
        .msg    = "Type inference: Types do not unify",
    });
    return false;
}

Subst* inferer_compose_substitutions(Subst* subst_a, Subst* subst_b)
{
    // Apply subst_a to subst_b
    // Then unify the two substitutions and return it.
}

bool inferer_bind_variable_to_type(Inferer* inferer, int var_id, Type* type, Subst** subst)
{
    assert(type  != NULL);
    assert(subst != NULL);

    *subst = NULL;
    bool var_id_in_type = false;
    int* free_vars = NULL ;

    if (type->kind == TYPE_FREE_VAR)
    {
        return true;
    }

    free_vars = inferer_get_free_type_vars_from_type(inferer, type);
    for (int i = 0; i < arrlen(free_vars); ++i)
    {
        int free_var = free_vars[i];
        if (free_var == var_id)
        {
            var_id_in_type = true;
            break;
        }
    }
    arrfree(free_vars);

    if (var_id_in_type)
    {
        inferer_throw_compiler_error(inferer, (CompileError)
        {
            .kind   = ERROR_ERROR,
            .line   = -1         ,
            .column = -1         ,
            .length = -1         ,
            .msg    = "Type inference: Occurs check fails.",
        });
        return false;
    }

    *subst = inferer_create_subst_singleton(inferer, var_id, type);
    return false;
}

// Probably the most central function to the inferer.
bool inferer_get_most_general_unifier(Inferer* inferer, Type* left, Type* right, Subst** subst)
{
    assert(left  != NULL);
    assert(right != NULL);
    assert(subst != NULL);

    *subst = NULL;

    switch (left->kind)
    {
        case TYPE_NIL        :
            if (right->kind == TYPE_NIL) return true;
            break;

        case TYPE_BOOL       :
            if (right->kind == TYPE_BOOL) return true;
            break;

        case TYPE_INT        :
            if (right->kind == TYPE_INT) return true;
            break;

        case TYPE_REAL       :
            if (right->kind == TYPE_REAL) return true;
            break;

        case TYPE_STRING     :
            if (right->kind == TYPE_STRING) return true;
            break;

        case TYPE_LIST       :
            if (right->kind == TYPE_LIST)
            {
                return inferer_get_most_general_unifier
                    (inferer, left->type.list.type, right->type.list.type, subst);
            }
            break;

        case TYPE_STRUCT     :
            assert(false);
            break;

        // TODO: Think about memory allocation system for substituions and other data.
        case TYPE_FN         :
            if (right->kind == TYPE_FN)
            {
                bool   result  = false;
                Subst* subst_a = NULL ;
                Subst* subst_b = NULL ;
                Subst* subst_ptr = NULL;

                result = inferer_get_most_general_unifier
                    (inferer, left->type.fn.left, right->type.fn.left, &subst_a);
                if (!result)
                {
                    return false;
                }

                inferer_apply_substitution_to_type(inferer, subst_a,  left->type.fn.right);
                inferer_apply_substitution_to_type(inferer, subst_b, right->type.fn.right);
                result = inferer_get_most_general_unifier
                    (inferer, left->type.fn.right, right->type.fn.right, &subst_b);
                if (!result)
                {
                    return false;
                }
                *subst = inferer_compose_substitutions(inferer, subst_a, subst_b);
                return true;
            }
            break;

        case TYPE_FREE_VAR   :
            assert(false);
            // *subst = inferer_get_free_type_vars_from_type(inferer, 
            break;

        case TYPE_BOUNDED_VAR:
            fprintf(stderr, "[%s:%d] Type inference: Bouded variable should not be present during unification.\n", __FILE__, __LINE__);
            exit(1);
            break;

        default:
            fprintf(stderr, "[%s:%d] Type inference: Failed to recongnize type kind.\n", __FILE__, __LINE__);
            exit(1);
    }

    return inferer_throw_failed_to_unify_error(inferer);
}
