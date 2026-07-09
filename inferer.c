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

// Free type vars are allocated using stb, not to an arena.
void type_get_free_type_vars(Type type, int** free_type_vars)
{
    assert(free_type_vars != NULL);
    assert(*free_type_vars == NULL);

    int* free_type_vars_left  = NULL;
    int* free_type_vars_right = NULL;

    switch (type->kind)
    {
        case TYPE_NIL        : return;
        case TYPE_BOOL       : return;
        case TYPE_INT        : return;
        case TYPE_REAL       : return;
        case TYPE_STRING     : return;

        case TYPE_LIST       :
            type_get_free_type_vars
                (inferer, type.type.list.type, free_type_vars);
            return;

        case TYPE_STRUCT     :
            assert(false);

        case TYPE_FN         :
            type_get_free_type_vars
                (inferer, type.type.fn.left , &free_type_vars_a);
            type_get_free_type_vars
                (inferer, type.type.fn.right, &free_type_vars_b);

            for (int i = 0; i < arrlen(free_type_vars_b); ++i)
            {
                arrput(free_type_vars_a, free_type_vars_b[i]);
            }
            arrfree(free_type_vars_b);

            *free_type_vars = free_type_vars_a;
            return;

        case TYPE_FREE_VAR   :
            arrput(*free_type_var, type.type.free_type.free_var_id);
            return;

        case TYPE_BOUNDED_VAR:
            // fprintf(stderr, "[%s:%d] Type inference: Bounded type var shouldn't have been found.\n", __FILE__, __LINE__);
            // exit(1);
            return;

        default:
            fprintf(stderr, "[%s:%d] Type inference: Failed to recognize type kind.\n", __FILE__, __LINE__);
            exit(1);
    }
}

void scheme_get_free_type_vars(Scheme scheme, int** free_type_vars)
{
    assert(free_type_vars != NULL);
    assert(*free_type_vars == NULL);

    type_get_free_type_vars
        (scheme.type, free_type_vars);
}

void type_env_get_free_type_vars(TypeEnv env, int** free_type_vars)
{
    assert(free_type_vars != NULL);
    assert(*free_type_vars == NULL);

    TypeEnvTerm* terms = env->terms;

    for (int i = 0; i < hmlen(terms); ++i)
    {
        TypeEnvTerm t = terms[i];
        scheme_get_free_type_vars
            (t.scheme, free_type_vars);
    }
}

Type type_deepcopy(Type* type)
{
    assert(false);
}

void substitute_var_free_with_bounded(Type* type, int free_var, int bounded_var)
{
    switch (type->kind)
    {
        case TYPE_NIL        : return;
        case TYPE_BOOL       : return;
        case TYPE_INT        : return;
        case TYPE_REAL       : return;
        case TYPE_STRING     : return;

        case TYPE_LIST       :
            substitute_var_free_with_bounded
                (type->type.list.type, free_var, bounded_var);
            return;

        case TYPE_STRUCT     :
            assert(false);

        case TYPE_FN         :
            substitute_var_free_with_bounded
                (type->type.fn.left , free_var, bounded_var);
            substitute_var_free_with_bounded
                (type->type.fn.right, free_var, bounded_var);
            return;

        case TYPE_FREE_VAR   :
            if (type->type.free_var.free_var_id == free_var)
            {
                *type = (Type)
                {
                    .kind = TYPE_BOUNDED_VAR,
                    .type.bounded_var = (TypeBoundedVar)
                    {
                        .bounded_var_id = bounded_var
                    }
                };
            }
            return;

        case TYPE_BOUNDED_VAR:
            fprintf(stderr, "[%s:%d] Type inference: Bounded type var shouldn't have been found.\n", __FILE__, __LINE__);
            exit(1);

        default:
            fprintf(stderr, "[%s:%d] Type inference: Failed to recongnize type kind.\n", __FILE__, __LINE__);
            exit(1);
    }
}

void type_apply_substitution(Subst subst, Type* type)
{
    switch (type->kind)
    {
        case TYPE_NIL        : return;
        case TYPE_BOOL       : return;
        case TYPE_INT        : return;
        case TYPE_REAL       : return;
        case TYPE_STRING     : return;

        case TYPE_LIST       :
            type_apply_substitution
                (inferer, subst, type->type.list.type);
            return;

        case TYPE_STRUCT     :
            assert(false);

        case TYPE_FN         :
            type_apply_substitution
                (inferer, subst, type->type.fn.left);
            type_apply_substitution
                (inferer, subst, type->type.fn.right);
            return;

        case TYPE_FREE_VAR   :
            for (int i = 0; i < arrlen(subst.substs); ++i)
            {
                SubstObj s = subst.substs[i];
                if (s.key == type->type.free_type.free_var_id)
                {
                    *type = type_deepcopy(s.value);
                    break;
                }
            }
            return;

        case TYPE_BOUNDED_VAR:
            fprintf(stderr, "[%s:%d] Type inference: Bounded type var shouldn't have been found.\n", __FILE__, __LINE__);
            exit(1);

        default:
            fprintf(stderr, "[%s:%d] Type inference: Failed to recongnize type kind.\n", __FILE__, __LINE__);
            exit(1);
    }
}

void type_env_add_term(TypeEnv* env, int key, Scheme value)
{
    assert(env != NULL);
    hmput(env->terms, key, value);
}

bool type_env_remove_term(TypeEnv* env, int key)
{
    assert(env != NULL);
    if (hmdel(env->terms, key))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void type_env_free(TypeEnv* env)
{
    assert(env != NULL);
    hmfree(env->terms);
}

// Type* is replaced.
void generalize_type(TypeEnv env, Type* type, Scheme* subst_ref)
{
    assert(type   != NULL);
    assert(scheme != NULL);

    int* free_type_vars = NULL;
    int* free_env_vars  = NULL;

    int* bounded_vars = NULL;
    int type_len = 0;
    int  env_len = 0;

    type_get_free_type_vars    (*type       , &free_type_vars);
    type_env_get_free_type_vars(env, &free_env_vars );

    type_len = free_type_vars == NULL ? 0 : arrlen(free_type_vars);
    env_len  = free_env_vars  == NULL ? 0 : arrlen(free_env_vars );

    // Not really efficient, but I don't think it needs to be here.
    for (int i = 0; i < env_len; ++i)
    {
        int env_var = free_env_vars[i];
        for (int j = 0; type_len; ++j)
        {
            int type_var = free_type_vars[j];
            if (env_var == type_var)
            {
                // Substitute all free variables with bounded ones
                substitute_var_free_with_bounded(type, type_var, type_var);
                // Increment bounded var num.
                arrput(bounded_vars, type_var);
            }
        }
    }
    arrfree(free_type_vars);
    arrfree(free_env_vars );

    *scheme = (Scheme)
    {
        .type         = type        ,
        .bounded_vars = bounded_vars,
    };
}

bool instantiate_scheme(Scheme scheme, Type* type_ref)
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

void subst_add_var(Subst* subst, int key, Type* value)
{
    assert(subst != NULL);

    hmput(subst->substs, key, value);
}

void subst_remove_var(Subst* subst, int key)
{
    assert(subst != NULL);

    hmdel(subst->substs, key);
}

// Allocates return type memory using stb.
Subst* compose_substitutions(Subst* subst_a, Subst* subst_b)
{
    // Apply subst_a to subst_b
    // Then unify the two substitutions and return it.
}

bool inferer_bind_variable_to_type(Inferer* inferer, int var_id, Type* type, Subst** subst)
{
    assert(type   != NULL);
    assert(subst  != NULL);
    assert(*subst == NULL);

    bool var_id_in_type = false;
    int* free_type_vars = NULL ;

    if (type->kind == TYPE_FREE_VAR)
    {
        return true;
    }

    if (!type_get_free_type_vars(inferer, type, &free_type_vars))
    {
        return false;
    }
    for (int i = 0; i < arrlen(free_vars); ++i)
    {
        int free_type_var = free_type_vars[i];
        if (free_type_var == var_id)
        {
            var_id_in_type = true;
            break;
        }
    }
    arrfree(free_type_vars);

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

    if (right->kind == TYPE_FREE_VAR)
        return inferer_bind_variable_to_type
            (inferer, right->type.free_var.free_var_id, left, subst);

    switch (left->kind)
    {
        case TYPE_NIL        : if (right->kind == TYPE_NIL   ) return true; break;
        case TYPE_BOOL       : if (right->kind == TYPE_BOOL  ) return true; break;
        case TYPE_INT        : if (right->kind == TYPE_INT   ) return true; break;
        case TYPE_REAL       : if (right->kind == TYPE_REAL  ) return true; break;
        case TYPE_STRING     : if (right->kind == TYPE_STRING) return true; break;

        case TYPE_LIST       :
            if (right->kind == TYPE_LIST)
                return inferer_get_most_general_unifier
                    (inferer, left->type.list.type, right->type.list.type, subst);

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

                type_apply_substitution(inferer, subst_a,  left->type.fn.right);
                type_apply_substitution(inferer, subst_b, right->type.fn.right);
                result = inferer_get_most_general_unifier
                    (inferer, left->type.fn.right, right->type.fn.right, &subst_b);
                if (!result)
                {
                    return false;
                }
                *subst = compose_substitutions(inferer, subst_a, subst_b);
                hmfree(subst_a);
                hmfree(subst_b);
                return true;
            }
            break;

        case TYPE_FREE_VAR   :
            return inferer_bind_variable_to_type
                (inferer, left->type.free_var.free_var_id, right, subst);
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
