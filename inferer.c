#include "inferer.h"

Type builtin_nil    = (Type) { .kind = TYPE_NIL    };
Type builtin_bool   = (Type) { .kind = TYPE_BOOL   };
Type builtin_int    = (Type) { .kind = TYPE_INT    };
// Type builtin_nat    = (Type) { .kind = TYPE_NAT    };
Type builtin_real   = (Type) { .kind = TYPE_REAL   };
Type builtin_string = (Type) { .kind = TYPE_STRING };

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

    switch (type.kind)
    {
        case TYPE_NIL        : return;
        case TYPE_BOOL       : return;
        case TYPE_INT        : return;
        case TYPE_REAL       : return;
        case TYPE_STRING     : return;

        case TYPE_LIST       :
            type_get_free_type_vars
                (*type.type.list.type, free_type_vars);
            return;

        case TYPE_STRUCT     :
            assert(false);

        case TYPE_FN         :
            type_get_free_type_vars
                (*type.type.fn.left , &free_type_vars_left );
            type_get_free_type_vars
                (*type.type.fn.right, &free_type_vars_right);

            for (int i = 0; i < arrlen(free_type_vars_left); ++i)
            {
                arrput(free_type_vars_left, free_type_vars_right[i]);
            }
            arrfree(free_type_vars_right);

            *free_type_vars = free_type_vars_left;
            return;

        case TYPE_FREE_VAR   :
            arrput(*free_type_vars, type.type.free_var.free_var_id);
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
        (*scheme.type, free_type_vars);
}

void type_env_get_free_type_vars(TypeEnvTerm* env, int** free_type_vars)
{
    assert(free_type_vars != NULL);
    assert(*free_type_vars == NULL);

    for (int i = 0; i < arrlen(env); ++i)
    {
        TypeEnvTerm t = env[i];
        scheme_get_free_type_vars
            (t.value, free_type_vars);
    }
}

Type* type_deepcopy(Arena* arena, Type* type)
{
    assert(arena != NULL);

    Type type_mem;

    if (type == NULL)
    {
        assert(false); // Idk if this is actaully a good idea
        return NULL;
    }

    assert(false);
    switch (type->kind)
    {
        case TYPE_NIL        : return (Type*) arena_push(arena, type, sizeof(Type));
        case TYPE_BOOL       : return (Type*) arena_push(arena, type, sizeof(Type));
        case TYPE_INT        : return (Type*) arena_push(arena, type, sizeof(Type));
        case TYPE_REAL       : return (Type*) arena_push(arena, type, sizeof(Type));
        case TYPE_STRING     : return (Type*) arena_push(arena, type, sizeof(Type));

        case TYPE_LIST       :
            type_mem = (Type)
            {
                .kind = TYPE_LIST,
                .type.list.type = type_deepcopy(arena, type->type.list.type), 
            };
            return (Type*) arena_push(arena, &type_mem, sizeof(Type));

        case TYPE_STRUCT     :
            assert(false);
            break;

        case TYPE_FN         :
            type_mem = (Type)
            {
                .kind = TYPE_FN,
                .type.fn = (TypeFn)
                {
                    .left  = type_deepcopy(arena, type->type.fn.left ),
                    .right = type_deepcopy(arena, type->type.fn.right),
                }
            };
            return (Type*) arena_push(arena, &type_mem, sizeof(Type));

        case TYPE_FREE_VAR   : return (Type*) arena_push(arena, type, sizeof(Type));
        case TYPE_BOUNDED_VAR: return (Type*) arena_push(arena, type, sizeof(Type));

        default:
            fprintf(stderr, "[%s:%d] Type inference: Couldn't recognize type kind while while copying type.\n", __FILE__, __LINE__);
            exit(1);
    }
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

void inferer_type_apply_substitution(Inferer* inferer, Subst* subst_arr, Type** type_ref)
{
    assert(subst_arr != NULL);
    assert(type_ref != NULL);
    assert(*type_ref != NULL);

    Type* type = *type_ref;

    switch (type->kind)
    {
        case TYPE_NIL        : return;
        case TYPE_BOOL       : return;
        case TYPE_INT        : return;
        case TYPE_REAL       : return;
        case TYPE_STRING     : return;

        case TYPE_LIST       :
            inferer_type_apply_substitution
                (inferer, subst_arr, &type->type.list.type);
            return;

        case TYPE_STRUCT     :
            assert(false);

        case TYPE_FN         :
            inferer_type_apply_substitution
                (inferer, subst_arr, &type->type.fn.left);
            inferer_type_apply_substitution
                (inferer, subst_arr, &type->type.fn.right);
            return;

        case TYPE_FREE_VAR   :
            for (int i = 0; i < arrlen(subst_arr); ++i)
            {
                Subst s = subst_arr[i];
                if (s.key == type->type.free_var.free_var_id)
                {
                    *type_ref = type_deepcopy(&inferer->type_arena, s.value);
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

// Type* is replaced.
void generalize_type(TypeEnvTerm* env, Type* type, Scheme* scheme)
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
        .type          = type        ,
        .bound_var_ids = bounded_vars,
    };
}

bool instantiate_scheme(Scheme scheme, Type* type_ref)
{
    assert(false);
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

// Allocates return type memory using stb.
Subst* compose_substitutions(Subst* subst_arr_a, Subst* subst_arr_b)
{
    Subst* subst_arr_new = NULL;

    for (int i = 0; i < arrlen(subst_arr_a); ++i)
    {
        Subst sa = subst_arr_a[i];
        arrput(subst_arr_new, sa);
    }
    for (int i = 0; i < arrlen(subst_arr_b); ++i)
    {
        Subst sb = subst_arr_b[i];
        arrput(subst_arr_new, sb);
    }

    return subst_arr_new;
}

bool inferer_bind_variable_to_type(Inferer* inferer, int var_id, Type* type, Subst** subst)
{
    assert(type   != NULL);
    assert(subst  != NULL);
    assert(*subst == NULL);

    bool var_id_in_type = false;
    int* free_type_vars = NULL ;
    Subst subst_mem;

    // type ~ TVar var_id
    if (type->kind == TYPE_FREE_VAR && var_id == type->type.free_var.free_var_id)
    {
        return true;
    }

    // Establish whether var_id 'part of' (ftv type)
    type_get_free_type_vars(*type, &free_type_vars);
    for (int i = 0; i < arrlen(free_type_vars); ++i)
    {
        int free_type_var = free_type_vars[i];
        if (free_type_var == var_id)
        {
            var_id_in_type = true;
            break;
        }
    }
    arrfree(free_type_vars);

    // If is part of, throw error.
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

    // otherwise
    // *subst = inferer_create_subst_singleton(inferer, var_id, type);
    subst_mem = (Subst)
    {
        .key   = var_id,
        .value = type  ,
    };
    arrput(*subst, subst_mem);
    return false;
}

Type* inferer_new_type_variable(Inferer* inferer)
{
    assert(false);
}

// Probably the most central function to the inferer.
bool inferer_get_most_general_unifier(Inferer* inferer, Type* left, Type* right, Subst** subst_arr)
{
    assert(left  != NULL);
    assert(right != NULL);
    assert(subst_arr != NULL);
    assert(*subst_arr == NULL);

    *subst_arr = NULL;

    if (right->kind == TYPE_FREE_VAR)
        return inferer_bind_variable_to_type
            (inferer, right->type.free_var.free_var_id, left, subst_arr);

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
                    (inferer, left->type.list.type, right->type.list.type, subst_arr);
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

                // TODO: Rethink this at a later date - should this modify the existing data types?
                inferer_type_apply_substitution(inferer, subst_a, & left->type.fn.right);
                inferer_type_apply_substitution(inferer, subst_a, &right->type.fn.right);
                result = inferer_get_most_general_unifier
                    (inferer, left->type.fn.right, right->type.fn.right, &subst_b);
                if (!result)
                {
                    return false;
                }
                *subst_arr = compose_substitutions(subst_a, subst_b);
                arrfree(subst_a);
                arrfree(subst_b);
                return true;
            }
            break;

        case TYPE_FREE_VAR   :
            return inferer_bind_variable_to_type
                (inferer, left->type.free_var.free_var_id, right, subst_arr);
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

Type* inferer_get_decl_type(Inferer* inferer, Decl* decl)
{
    assert(false);
}

bool inferer_infer_expr_primary(Inferer* inferer, Expr* expr, Type** type_ref, Subst** subst_ref)
{
    assert(inferer   != NULL);
    assert(expr      != NULL);
    assert(type_ref  != NULL);
    assert(*type_ref != NULL);

    Type* type = *type_ref;
    Type  type_mem;

    switch (expr->expr.primary.kind)
    {
        case EXPR_PRIMARY_UNKNOWN   :
            fprintf(stderr, "[%s:%d] Type inference: Failed to recognize primary expr kind.\n", __FILE__, __LINE__);
            exit(1);

        case EXPR_PRIMARY_NIL       :
            type_mem = (Type) { .kind = TYPE_NIL };
            *type_ref = (Type*) arena_push(&inferer->type_arena, &type_mem, sizeof(Type));
            return true;

        case EXPR_PRIMARY_BOOLEAN   :
            type_mem = (Type) { .kind = TYPE_NIL };
            *type_ref = (Type*) arena_push(&inferer->type_arena, &type_mem, sizeof(Type));
            return true;

        case EXPR_PRIMARY_STRING    :
            type_mem = (Type) { .kind = TYPE_NIL };
            *type_ref = (Type*) arena_push(&inferer->type_arena, &type_mem, sizeof(Type));
            return true;

        case EXPR_PRIMARY_NATURAL   :
            type_mem = (Type) { .kind = TYPE_NIL };
            *type_ref = (Type*) arena_push(&inferer->type_arena, &type_mem, sizeof(Type));
            return true;

        case EXPR_PRIMARY_INTEGER   :
            type_mem = (Type) { .kind = TYPE_NIL };
            *type_ref = (Type*) arena_push(&inferer->type_arena, &type_mem, sizeof(Type));
            return true;

        case EXPR_PRIMARY_REAL      :
            type_mem = (Type) { .kind = TYPE_NIL };
            *type_ref = (Type*) arena_push(&inferer->type_arena, &type_mem, sizeof(Type));
            return true;

        case EXPR_PRIMARY_LIST      :
            assert(false);
            break;

        case EXPR_PRIMARY_STRUCT    :
            assert(false);
            break;

        case EXPR_PRIMARY_FN        :
            assert(false);
            break;

        case EXPR_PRIMARY_IDENTIFIER:
            fprintf(stderr, "[%s:%d] Type inference: Found unresolved identifier in type inference stage of compilation.\n", __FILE__, __LINE__);
            exit(1);

        case EXPR_PRIMARY_DECL      :
            type = inferer_get_decl_type(inferer, expr->expr.primary.primary.decl);
            assert(type != NULL);
            *type_ref = type_deepcopy(&inferer->type_arena, type);
            return true;

        default:
            fprintf(stderr, "[%s:%d] Type inference: Failed to recognize primary expr kind.\n", __FILE__, __LINE__);
            exit(1);
    }
}

bool inferer_infer_expr_unary_operator(Inferer* inferer, Expr* operand, Type* builtin_type, Type** type_ref, Subst** subst_ref)
{
    assert(inferer   != NULL);
    assert(operand   != NULL);
    assert(builtin_type != NULL);
    assert(type_ref  != NULL);
    assert(*type_ref != NULL);
    assert(subst_ref != NULL);
    assert(*subst_ref != NULL);

    Type* unary_type = NULL;

    Subst* subst_a = NULL;
    Subst* subst_b = NULL;
    Subst* unified_subst = NULL;

    inferer_infer_expr(inferer, operand, &unary_type, &subst_a);

    inferer_type_apply_substitution(inferer, subst_a, &unary_type);

    if (!inferer_get_most_general_unifier(inferer, unary_type, builtin_type, &subst_b))
    {
        return false;
    }

    unified_subst = compose_substitutions(subst_b, subst_a);

    inferer_type_apply_substitution(inferer, unified_subst, &unary_type);
    arrfree(subst_a);
    arrfree(subst_b);

    *type_ref  = unary_type   ;
    *subst_ref = unifier_subst;

    return true;
}

bool inferer_infer_expr_unary(Inferer* inferer, Expr* expr, Type** type_ref, Subst** subst_ref)
{
    assert(inferer   != NULL);
    assert(expr      != NULL);
    assert(type_ref  != NULL);
    assert(*type_ref != NULL);

    switch (expr->expr.unary.kind)
    {
        case EXPR_UNARY_UNKNOWN:
            fprintf(stderr, "[%s:%d] Type inference: Failed to recognize unary expr kind.\n", __FILE__, __LINE__);
            exit(1);

        case EXPR_UNARY_NOT    :
            return inferer_infer_expr_unary_operator
                (inferer, expr->expr.unary.unary, &builtin_bool, type_ref, subst_ref)

        case EXPR_UNARY_NEGATE :
            return inferer_infer_expr_unary_operator
                (inferer, expr->expr.unary.unary, &builtin_int , type_ref, subst_ref)

        default:
            fprintf(stderr, "[%s:%d] Type inference: Failed to recognize unary expr kind.\n", __FILE__, __LINE__);
            exit(1);
    }
}

bool inferer_infer_expr_binary_operator(Inferer* inferer, Expr* left, Expr* right, Type* left_builtin_type, Type* right_builtin_type, Type** type_ref, Subst** subst_ref)
{
    assert(inferer   != NULL);
    assert(left      != NULL);
    assert(right     != NULL);
    assert(left_builtin_type  != NULL);
    assert(right_builtin_type != NULL);
    assert(type_ref  != NULL);
    assert(*type_ref != NULL);
    assert(subst_ref != NULL);
    assert(*subst_ref != NULL);

    Type* type_var   = NULL;
    Type* left_type  = NULL;
    Type* right_type = NULL;

    Subst* left_subst  = NULL;
    Subst* right_subst = NULL;


    // ================================================================================


    // Infer left hand side of binary expression.
    inferer_infer_expr(inferer, left, &left_type, &left_subst);

    // Substitute all type variables in type environment with types.
    inferer_type_env_apply_substitution(inferer, left_subst);

    inferer_get_most_general_unifier(inferer, left_type, left_builtin_type, &left_subst);

    inferer_type_env_apply_substitution(inferer, left_subst);

    // ================================================================================


    // Infer right hand side of binary expression.
    infer_inferer_expr(inferer, right, &right_type, &right_subst);

    // Substitute all type variables in type environment with types.
    inferer_type_env_apply_substitution(inferer, right_subst);

    inferer_get_most_general_unifier(inferer, right_type, right_builtin_type, &right_subst);

    inferer_type_env_apply_substitution(inferer, right_subst);

    // ================================================================================

    return true;
}

bool inferer_infer_expr_binary(Inferer* inferer, Expr* expr, Type** type_ref, Subst** subst_ref)
{
    assert(inferer   != NULL);
    assert(expr      != NULL);
    assert(type_ref  != NULL);
    assert(*type_ref != NULL);

    Type* type = *type_ref;
    Type  type_mem;

    switch (expr->expr.binary.kind)
    {
        case EXPR_BINARY_UNKNOWN      :
            fprintf(stderr, "[%s:%d] Type inference: Failed to recognize binary expr kind.\n", __FILE__, __LINE__);
            exit(1);

        case EXPR_BINARY_ADD          :
            assert(false);
            break;

        case EXPR_BINARY_SUBTRACT     :
            assert(false);
            break;

        case EXPR_BINARY_MULTIPLY     :
            assert(false);
            break;

        case EXPR_BINARY_DIVIDE       :
            assert(false);
            break;

        case EXPR_BINARY_MODULO       :
            assert(false);
            break;

        case EXPR_BINARY_AND          :
            assert(false);
            break;

        case EXPR_BINARY_OR           :
            assert(false);
            break;

        case EXPR_BINARY_EQUAL        :
            assert(false);
            break;

        case EXPR_BINARY_NOT_EQUAL    :
            assert(false);
            break;

        case EXPR_BINARY_LESS_EQUAL   :
            assert(false);
            break;

        case EXPR_BINARY_LESS         :
            assert(false);
            break;

        case EXPR_BINARY_GREATER_EQUAL:
            assert(false);
            break;

        case EXPR_BINARY_GREATER      :
            assert(false);
            break;

        case EXPR_BINARY_CHAIN        :
            assert(false);
            break;

        case EXPR_BINARY_ACCESS       :
            assert(false);
            break;

        case EXPR_BINARY_ASSIGN       :
            assert(false);
            break;

        case EXPR_BINARY_INDEX        :
            assert(false);
            break;

        default:
            fprintf(stderr, "[%s:%d] Type inference: Failed to recognize binary expr kind.\n", __FILE__, __LINE__);
            exit(1);
    }
}

bool inferer_infer_expr_fn(Inferer* inferer, Expr* expr, Type** type_ref, Subst** subst_ref)
{
    assert(inferer   != NULL);
    assert(expr      != NULL);
    assert(type_ref  != NULL);
    assert(*type_ref != NULL);

    Type* type = *type_ref;
    Type  type_mem;

    fprintf(stderr, "[%s:%d] Type inference: Failed to infer expression of type fn.\n", __FILE__, __LINE__);
    exit(1);
}

bool inferer_infer_expr(Inferer* inferer, Expr* expr, Type** type_ref, Subst** subst_ref)
{
    assert(inferer   != NULL);
    assert(expr      != NULL);
    assert(type_ref  != NULL);
    assert(*type_ref != NULL);

    Type* type = *type_ref;

    switch(expr->kind)
    {
        case EXPR_PRIMARY: return inferer_infer_expr_primary (inferer, expr, type_ref, subst_ref);
        case EXPR_UNARY  : return inferer_infer_expr_unary   (inferer, expr, type_ref, subst_ref);
        case EXPR_BINARY : return inferer_infer_expr_binary  (inferer, expr, type_ref, subst_ref);
        case EXPR_FN     : return inferer_infer_expr_fn      (inferer, expr, type_ref, subst_ref);
        default:
            fprintf(stderr, "[%s:%d] Type inference: Failed to recognize expr kind.\n", __FILE__, __LINE__);
            exit(1);
    }
}

void inferer_throw_compiler_error(Inferer* inferer, CompileError err)
{
    CompileError* err_ptr = NULL;
    err_ptr = (CompileError*) arena_push(&inferer->arena, &err, sizeof(CompileError));
    arrput(inferer->errs, err_ptr);
}
