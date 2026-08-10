#include "inferer.h"

static Type private_builtin_type_nil     = (Type) { .kind = TYPE_NIL     };
static Type private_builtin_type_bool    = (Type) { .kind = TYPE_BOOL    };
static Type private_builtin_type_nat     = (Type) { .kind = TYPE_NAT     };
static Type private_builtin_type_int     = (Type) { .kind = TYPE_INT     };
static Type private_builtin_type_real    = (Type) { .kind = TYPE_REAL    };
static Type private_builtin_type_string  = (Type) { .kind = TYPE_STRING  };

static Type* builtin_type_nil     = &private_builtin_type_nil    ;
static Type* builtin_type_bool    = &private_builtin_type_bool   ;
static Type* builtin_type_nat     = &private_builtin_type_nat    ;
static Type* builtin_type_int     = &private_builtin_type_int    ;
static Type* builtin_type_real    = &private_builtin_type_real   ;
static Type* builtin_type_string  = &private_builtin_type_string ;

Inferer inferer_init(Resolver* resolver)
{
    assert(resolver != NULL);

    Inferer inferer = (Inferer)
    {
        .filename       = resolver->filename,
        .txt            = resolver->txt   ,
        .tokens         = resolver->tokens,
        .stmts          = resolver->stmts ,
        .declarations   = resolver->declarations,
        .identifiers    = resolver->identifiers ,
        .arena          = resolver->arena,
        .type_arena     = NULL,
        .type_variables = NULL,
        .binds          = NULL,
        .diagnostic_component = diagnostic_component_init(),
    };

    arrfree(resolver->context);
    diagnostic_component_free(&resolver->diagnostic_component);

    *resolver = (Resolver)
    {
        .filename         = NULL ,
        .txt              = NULL ,
        .tokens           = NULL ,
        .stmts            = NULL ,
        .arena            = NULL ,
        .decl_id          = 0    ,
        .type_variable_id = 0    ,
        .loop_depth       = 0    ,
        // .inside_function  = false,
        .curr_fn          = NULL ,
        .context          = NULL ,
        .declarations     = NULL ,
        .identifiers      = NULL ,
        .diagnostic_component = NULL ,
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
    arrfree(inferer->identifiers );
    arrfree(inferer->type_variables);
    arrfree(inferer->binds);
    diagnostic_component_free(&inferer->diagnostic_component);

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
        .binds          = NULL,
        .diagnostic_component = NULL,
    };
}

bool inferer_unify_left_type_numeric(Inferer* inferer, Type** left_ref, Type** right_ref)
{
    assert(inferer    != NULL);
    assert(left_ref   != NULL);
    assert(*left_ref  != NULL);
    assert(right_ref  != NULL);
    assert(*right_ref != NULL);
    assert(type_kind_is_numeric((*left_ref)->kind));

    Type* left  = *left_ref ;
    Type* right = *right_ref;

    if (left->kind != right->kind)
    {
        return false;
    }

    return true;
}

bool inferer_unify_inner_left_type_list(Inferer* inferer, TypeList left_list, Type** right_ref)
{
    assert(inferer    != NULL);
    assert(right_ref  != NULL);
    assert(*right_ref != NULL);

    bool is_successful = false;
    Type* right = *right_ref;

    if (right->kind == TYPE_LIST)
    {
        is_successful = inferer_unify_inner(inferer, &left_list.type, &right->type.list.type);
        if (!is_successful)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    return true;
}

bool inferer_unify_inner_left_type_struct(Inferer* inferer, TypeStruct left_struct, Type** right_ref)
{
    assert(inferer    != NULL);
    assert(right_ref  != NULL);
    assert(*right_ref != NULL);

    bool is_successful = false;
    Type* right = *right_ref;
    TypeStruct right_struct;

    if (right->kind != TYPE_STRUCT)
    {
        return false;
    }
    right_struct = right->type.structt;

    // TODO: This (the rest of the function) could be made more efficient.
    for (int i = 0; i < left_struct.field_num; ++i)
    {
        TypeStructField* left_field  = left_struct.fields[i];
        TypeStructField* right_field = type_struct_find_key(right_struct, left_field->key); 

        if (right_field == NULL)
        {
            return false;
        }

        is_successful = inferer_unify_inner(inferer, &left_field->value, &right_field->value);
        if (!is_successful)
        {
            return false;
        }
    }

    // Afterwards, we check whether the right struct contains excess keys.
    for (int i = 0; i < right_struct.field_num; ++i)
    {
        TypeStructField* right_field = right_struct.fields[i];
        TypeStructField* left_field  = type_struct_find_key(left_struct , right_field->key); 

        if (left_field == NULL)
        {
            return false;
        }
    }

    return true;
}

bool inferer_unify_inner_left_type_fn(Inferer* inferer, TypeFn left_fn, Type** right_ref)
{
    assert(inferer    != NULL);
    assert(right_ref  != NULL);
    assert(*right_ref != NULL);

    bool is_successful = false;
    Type* right = *right_ref;

    if (right->kind == TYPE_FN)
    {
        is_successful = inferer_unify_inner(inferer, &left_fn.left, &right->type.fn.left);
        if (!is_successful)
        {
            return false;
        }

        is_successful = inferer_unify_inner(inferer, &left_fn.right, &right->type.fn.right);
        if (!is_successful)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    return true;
}

bool inferer_unify_inner_left_type_application(Inferer* inferer, TypeApplication left_application, Type** right_ref)
{
    assert(inferer    != NULL);
    assert(right_ref  != NULL);
    assert(*right_ref != NULL);

    bool is_successful = false;
    Type* right = *right_ref;

    if (right->kind == TYPE_APPLICATION && inferer_type_applications_are_equal(inferer, left_application, right->type.application))
    {
        // If the applications are equal, then so are the 'argc's
        for (int i = 0; i < left_application.argc; ++i)
        {
            Type*  left_type = left_application.argv[i];
            Type* right_type = right->type.application.argv[i];

            is_successful = inferer_unify_inner(inferer, &left_type, &right_type);
            if (!is_successful)
            {
                return false;
            }
        }
    }
    else
    {
        return false;
    }

    return true;
}

bool inferer_unify_inner_left_type_constructor(Inferer* inferer, TypeConstructor left_constructor, Type** right_ref)
{
    assert(inferer    != NULL);
    assert(right_ref  != NULL);
    assert(*right_ref != NULL);

    bool is_successful = false;
    Type* right = *right_ref;

    if (right->kind == TYPE_CONSTRUCTOR)
    {
        is_successful = inferer_unify_inner(inferer, &left_constructor.left, &right->type.constructor.left);
        if (!is_successful)
        {
            return false;
        }

        is_successful = inferer_unify_inner(inferer, &left_constructor.right, &right->type.constructor.right);
        if (!is_successful)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    return true;
}

bool inferer_unify_inner_left_type_alias(Inferer* inferer, TypeAlias left_alias, Type** right_ref)
{
    assert(inferer    != NULL);
    assert(right_ref  != NULL);
    assert(*right_ref == NULL);

    Type* right = *right_ref;

    if (right->kind == TYPE_ALIAS)
    {
        if (!inferer_unify_inner(inferer, &left_alias.type, &right->type.alias.type))
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    return true;
}

// Attempts to attempt_unify two types
// Returns:
//     * 'true'  on successful unification,
//     makes changes to both the left and right types.
//     * 'false' on failure to attempt_unify,
//     does not modify the types on either the left or right
// TODO: Change the way 'TYPE_ALIAS' is unified.
bool inferer_unify_inner(Inferer* inferer, Type** left_ref, Type** right_ref)
{
    assert(inferer    != NULL);
    assert(left_ref   != NULL);
    assert(right_ref  != NULL);
    assert(*left_ref  != NULL);
    assert(*right_ref != NULL);

    Type* left  = *left_ref ;
    Type* right = *right_ref;

    if (right->kind == TYPE_FREE_VAR)
    {
        return inferer_bind_variable_to_type(inferer, right_ref, left);
    }

    // TODO: Rewrite this switch-case to account for attempt_unifying numerics properly.
    switch (left->kind)
    {
        case TYPE_NAT        :
        case TYPE_INT        :
        case TYPE_REAL       :
            return inferer_unify_left_type_numeric(inferer, left_ref, right_ref);

        case TYPE_NIL        : return right->kind == TYPE_NIL        ;
        case TYPE_BOOL       : return right->kind == TYPE_BOOL       ;
        case TYPE_STRING     : return right->kind == TYPE_STRING     ;
        case TYPE_LIST       : return inferer_unify_inner_left_type_list   (inferer, left->type.list   , right_ref);
        case TYPE_STRUCT     : return inferer_unify_inner_left_type_struct (inferer, left->type.structt, right_ref);
        case TYPE_FN         : return inferer_unify_inner_left_type_fn     (inferer, left->type.fn     , right_ref);

        case TYPE_FREE_VAR   : return inferer_bind_variable_to_type(inferer, left_ref, right);
        case TYPE_BOUNDED_VAR: UNREACHABLE; // Should be instantiated before this point.

        case TYPE_APPLICATION:
            return inferer_unify_inner_left_type_application
                (inferer, left->type.application, right_ref);

        case TYPE_CONSTRUCTOR:
            return inferer_unify_inner_left_type_constructor
                (inferer, left->type.constructor, right_ref);

        case TYPE_ALIAS      :
            return inferer_unify_inner_left_type_alias
                (inferer, left->type.alias      , right_ref);
    }

    return false;
}

bool inferer_unify(Inferer* inferer, Type** left_ref, Type** right_ref, Span span)
{
    assert(inferer    != NULL);
    assert(left_ref   != NULL);
    assert(right_ref  != NULL);
    assert(*left_ref  != NULL);
    assert(*right_ref != NULL);

    if (!inferer_unify_inner(inferer, left_ref, right_ref))
    {
        inferer_unify_free_binds(inferer);
        inferer_throw_err_unify_failed(inferer, span, *left_ref, *right_ref);
        return false;
    }

    inferer_unify_apply_binds(inferer);
    inferer_unify_free_binds (inferer);
    return true;
}

bool inferer_attempt_unify(Inferer* inferer, Type** left_ref, Type** right_ref)
{
    assert(inferer    != NULL);
    assert(left_ref   != NULL);
    assert(right_ref  != NULL);
    assert(*left_ref  != NULL);
    assert(*right_ref != NULL);

    if (inferer_unify_inner(inferer, left_ref, right_ref))
    {
        inferer_unify_apply_binds(inferer);
        inferer_unify_free_binds (inferer);
        return true;
    }

    return false;
}

// The function 'inferer_generalize' abstracts a type over all type variables
// which are free in the type but not free in the given type environment.
void inferer_generalize(Inferer* inferer, Type* type, TypeScheme** scheme)
{
    assert(inferer != NULL);
    assert(type    != NULL);
    assert(scheme  != NULL);

    TypeScheme scheme_mem;
    HASHMAP(Subst*) substs = NULL;
    int quantified_count = 0;

    inferer_get_free_var_substs(inferer, type, &substs);

    for (int i = 0; i < hmlen(substs); ++i)
    {
        Subst subst = substs[i];
        if (inferer_subst_is_free_in_type_env(inferer, subst))
        {
            Type free_type = (Type)
            {
                .kind = TYPE_BOUNDED_VAR,
                .type.bounded_var = (TypeBoundedVar) { .id = quantified_count++ }
            };
            subst.value = (Type*) arena_push(&inferer->type_arena, &free_type, sizeof(Type));
            inferer_apply_subst(inferer, &type, subst);
        }
    }

    scheme_mem = (TypeScheme)
    {
        .quantified_count = quantified_count,
        .type = type,
    };
    hmfree(substs);

    *scheme = (TypeScheme*) arena_push(&inferer->type_arena, &scheme_mem, sizeof(TypeScheme));
}

Type* inferer_instantiate_type(Inferer* inferer, Type* type, DYNAMIC_ARRAY(Type**) quantified)
{
    assert(inferer != NULL);
    assert(type != NULL);

    Type copy;

    switch (type->kind)
    {
        case TYPE_NIL:
        case TYPE_BOOL:
        case TYPE_NAT:
        case TYPE_INT:
        case TYPE_REAL:
        case TYPE_STRING:
            return type;

        case TYPE_FREE_VAR:
            copy = *type;
            break;

        case TYPE_BOUNDED_VAR:
            return quantified[type->type.bounded_var.id];

        case TYPE_LIST:
            copy.kind = TYPE_LIST;
            copy.type.list.type =
                inferer_instantiate_type
                    (inferer, type->type.list.type, quantified);
            break;

        case TYPE_FN:
            copy.kind = TYPE_FN;
            copy.type.fn.left =
                inferer_instantiate_type
                    (inferer, type->type.fn.left, quantified);

            copy.type.fn.right =
                inferer_instantiate_type
                    (inferer, type->type.fn.right, quantified);
            break;

        case TYPE_STRUCT:
        {
            copy.kind = TYPE_STRUCT;
            copy.type.structt.field_num =
                type->type.structt.field_num;

            DYNAMIC_ARRAY(TypeStructField**) fields = NULL;

            for (int i = 0; i < type->type.structt.field_num; ++i)
            {
                TypeStructField* old =
                    type->type.structt.fields[i];

                TypeStructField field_copy =
                {
                    .key = old->key,
                    .value = inferer_instantiate_type
                        (inferer, old->value, quantified),
                };

                arrput(fields,
                    arena_push
                       (&inferer->type_arena, &field_copy, sizeof(TypeStructField)));
            }

            TypeStructField** tmp_ptr = fields;
            fields = (TypeStructField**) arena_push(&inferer->type_arena, tmp_ptr, type->type.structt.field_num * sizeof(TypeStructField*));
            arrfree(tmp_ptr);

            copy.type.structt.fields = fields;
            break;
        }

        case TYPE_APPLICATION:
        {
            copy.kind = TYPE_APPLICATION;
            copy.type.application = type->type.application;

            for (int i = 0; i < copy.type.application.argc; ++i)
            {
                copy.type.application.argv[i] = inferer_instantiate_type
                    (inferer, copy.type.application.argv[i], quantified);
            }

            break;
        }

        case TYPE_CONSTRUCTOR:
            copy.kind = TYPE_CONSTRUCTOR;
            copy.type.constructor.left =
                inferer_instantiate_type
                    (inferer, type->type.constructor.left, quantified);

            copy.type.constructor.right =
                inferer_instantiate_type
                    (inferer, type->type.constructor.right, quantified);
            break;

        case TYPE_ALIAS:
            copy.kind = TYPE_ALIAS;
            copy.type.alias.type =
                inferer_instantiate_type
                    (inferer, type->type.alias.type, quantified);
            break;
    }

    return arena_push
        (&inferer->type_arena, &copy, sizeof(Type));
}

void inferer_instantiate(Inferer* inferer, TypeScheme* scheme, Type** type)
{
    assert(inferer != NULL);
    assert(scheme != NULL);
    assert(type != NULL);
    assert(*type == NULL);

    Type** quantified = NULL;

    for (int i = 0; i < scheme->quantified_count; ++i)
    {
        arrput(quantified, inferer_create_free_type_var(inferer));
    }

    *type = inferer_instantiate_type
        (inferer, scheme->type, quantified);

    arrfree(quantified);
}

bool inferer_constrain_numeric(Inferer* inferer, Type** type_ref, Span span)
{
    assert(inferer   != NULL);
    assert(type_ref  != NULL);
    assert(*type_ref != NULL);

    Type* type = *type_ref;

    if (!type_kind_is_numeric(type->kind))
    {
        inferer_throw_err_type_isnt_numeric(inferer, span);
        return false;
    }

    return true;
}

bool inferer_constrain_equality(Inferer* inferer, Type** type_ref, Span span)
{
    assert(inferer   != NULL);
    assert(type_ref  != NULL);
    assert(*type_ref != NULL);

    Type* type = *type_ref;

    if (!type_kind_is_equality(type->kind))
    {
        inferer_throw_err_type_isnt_equality(inferer, span);
        return false;
    }

    return true;
}

void inferer_get_free_var_substs(Inferer* inferer, Type* type, HASHMAP(Subst*)* substs)
{
    assert(inferer != NULL);
    assert(type    != NULL);
    assert(substs  != NULL);

    Type free_type;

    switch(type->kind)
    {
        case TYPE_NIL        :
        case TYPE_BOOL       :
        case TYPE_NAT        :
        case TYPE_INT        :
        case TYPE_REAL       :
        case TYPE_STRING     :
            return;

        case TYPE_LIST       : inferer_get_free_var_substs(inferer, type->type.list.type, substs); return;
        case TYPE_STRUCT     :
            for (int i = 0; i < type->type.structt.field_num; ++i)
            {
                TypeStructField* field = type->type.structt.fields[i];
                inferer_get_free_var_substs(inferer, field->value, substs);
            }
            return;

        case TYPE_FN         :
            if (type->type.fn.left != NULL)
            {
                // This is so that functions that take no arguments are allowed.
                inferer_get_free_var_substs(inferer, type->type.fn.left , substs);
            }
            inferer_get_free_var_substs(inferer, type->type.fn.right, substs);
            return;

        case TYPE_FREE_VAR   :
            free_type = (Type)
            {
                .kind = TYPE_BOUNDED_VAR,
                .type.bounded_var = (TypeBoundedVar) { .id = hmlen(*substs) }
            };
            hmput(*substs, type, (Type*) arena_push(&inferer->type_arena, &free_type, sizeof(Type)));
            return;

        case TYPE_BOUNDED_VAR: UNREACHABLE;

        case TYPE_APPLICATION:
            for (int i = 0; i < type->type.application.argc; ++i)
            {
                Type* arg = type->type.application.argv[i];

                inferer_get_free_var_substs(inferer, arg, substs);
            }
            return;

        case TYPE_CONSTRUCTOR: inferer_get_free_var_substs(inferer, type->type.constructor.left , substs);
                               inferer_get_free_var_substs(inferer, type->type.constructor.right, substs);
                               return;

        case TYPE_ALIAS      : inferer_get_free_var_substs(inferer, type->type.alias.type, substs); return;
    }
    UNREACHABLE;
}

// TODO: Simplify this and reverse subst functions.
void inferer_apply_subst(Inferer* inferer, Type** type_ref, Subst subst)
{
    assert(inferer   != NULL);
    assert(type_ref  != NULL);
    assert(*type_ref != NULL);

    Type* type = *type_ref;

    switch (type->kind)
    {
        case TYPE_NIL        :
        case TYPE_BOOL       :
        case TYPE_NAT        :
        case TYPE_INT        :
        case TYPE_REAL       :
        case TYPE_STRING     :
            return;

        case TYPE_LIST       : inferer_apply_subst(inferer, &type->type.list.type, subst); return;
        case TYPE_STRUCT     :
            for (int i = 0; i < type->type.structt.field_num; ++i)
            {
                TypeStructField* field = type->type.structt.fields[i];
                inferer_apply_subst(inferer, &field->value, subst);
            }
            return;

        case TYPE_FN         : inferer_apply_subst(inferer, &type->type.fn.left , subst);
                               inferer_apply_subst(inferer, &type->type.fn.right, subst);
                               return;

        case TYPE_FREE_VAR   :
        case TYPE_BOUNDED_VAR:
            if (type == subst.key)
            {
                *type_ref = subst.value;
            }
            return;

        case TYPE_APPLICATION:
            for (int i = 0; i < type->type.application.argc; ++i)
            {
                Type* arg = type->type.application.argv[i];

                inferer_apply_subst(inferer, &arg, subst);
            }
            return;

        case TYPE_CONSTRUCTOR: inferer_apply_subst(inferer, &type->type.constructor.left , subst);
                               inferer_apply_subst(inferer, &type->type.constructor.right, subst);
                               return;

        case TYPE_ALIAS      : inferer_apply_subst(inferer, &type->type.alias.type, subst); return;
    }
    UNREACHABLE;
}

void inferer_decl_apply_subst(Inferer* inferer, Decl* decl, Subst subst)
{
    assert(inferer != NULL);
    assert(decl    != NULL);

    switch (decl->kind)
    {
        case DECL_VAR             :
            if (decl->decl.var.type != NULL)
            {
                inferer_apply_subst(inferer, &decl->decl.var.type     , subst);
            }
            return; 

        case DECL_TYPE_VAR        :
            if (decl->decl.type_var.type != NULL)
            {
                inferer_apply_subst(inferer, &decl->decl.type_var.type, subst);
            }
            return;

        // Probably doesn't need that, since a type is supposed to be self_contained.
        // Same goes for the type constructor.
        case DECL_ALIAS           : return;
        case DECL_TYPE            : return;
        case DECL_TYPE_CONSTRUCTOR: return;
    }
    UNREACHABLE;
}

bool inferer_infer_expr_primary_list(Inferer* inferer, Expr* expr, Type** type)
{
    assert(inferer != NULL);
    assert(expr != NULL);
    assert(expr->kind == EXPR_PRIMARY);
    assert(expr->expr.primary.kind == EXPR_PRIMARY_LIST);
    assert(type    != NULL);
    assert(*type   == NULL);

    ExprPrimaryList list = expr->expr.primary.primary.list;
    Type* inferred_type = NULL;

    for (int i = 0; i < list.length; ++i)
    {
        Expr* curr_expr = list.list[i];
        Type* curr_type = NULL;

        if (!inferer_infer_expr(inferer, curr_expr, &curr_type))
        {
            return false;
        }

        if (inferred_type == NULL)
        {
            inferred_type = curr_type;
        }
        else if (!inferer_unify(inferer, &inferred_type, &curr_type, inferer_get_expr_span(inferer, expr)))
        {
            return false;
        }
    }

    if (inferred_type == NULL)
    {
        inferred_type = inferer_create_free_type_var(inferer);
    }

    *type = inferer_create_list_type(inferer, inferred_type);
    return true;
}

// TODO: Check if this is fully correct.
bool inferer_infer_expr_primary_struct(Inferer* inferer, Expr* expr, Type** type)
{
    assert(inferer != NULL);
    assert(expr != NULL);
    assert(expr->kind == EXPR_PRIMARY);
    assert(expr->expr.primary.kind == EXPR_PRIMARY_STRUCT);
    assert(type    != NULL);
    assert(*type   == NULL);

    ExprPrimaryStruct structt = expr->expr.primary.primary.structt;
    Type type_mem;
    int field_num = 0;
    DYNAMIC_ARRAY(TypeStructField** fields) = NULL;

    for (int i = 0; i < structt.argc; ++i)
    {
        ExprPrimaryStructField* curr_field = structt.argv[i];
        TypeStructField  curr_type_field;
        TypeStructField* pushed_field = NULL;
        Type* curr_type = NULL;

        curr_type_field.key = curr_field->key;
        inferer_infer_expr(inferer, curr_field->value, &curr_type);

        if (curr_field->type != NULL)
        {
            Type* converted_type = NULL;

            inferer_convert_type_expr(inferer, curr_field->type, &converted_type);

            if (!inferer_unify(inferer, &converted_type, &curr_type, inferer_get_expr_span(inferer, expr)))
            {
                return false;
            }
        }

        curr_type_field.value = curr_type;
        pushed_field = (TypeStructField*) arena_push(&inferer->type_arena, &curr_type_field, sizeof(TypeStructField));
        arrput(fields, pushed_field);
    }

    TypeStructField** tmp_ptr = fields;
    field_num = arrlen(tmp_ptr);
    fields = (TypeStructField**) arena_push(&inferer->type_arena, tmp_ptr, field_num * sizeof(TypeStructField*));
    arrfree(tmp_ptr);

    type_mem = (Type)
    {
        .kind = TYPE_STRUCT,
        .type.structt = (TypeStruct)
        {
            .field_num = field_num,
            .fields    = fields   ,
        }
    };

    *type = (Type*) arena_push(&inferer->type_arena, &type_mem, sizeof(Type));
    return true;
} 

// Lambda functions are not yet implemented in this language.
bool inferer_infer_expr_primary_lambda(Inferer* inferer, Expr* expr, Type** type)
{
    assert(inferer != NULL);
    assert(expr != NULL);
    assert(expr->kind == EXPR_PRIMARY);
    assert(expr->expr.primary.kind == EXPR_PRIMARY_LAMBDA);
    assert(type    != NULL);
    assert(*type   == NULL);
    assert(expr->expr.primary.primary.lambda.decl->kind == DECL_VAR);

    TypeEnv type_env;
    ExprPrimaryLambda lambda = expr->expr.primary.primary.lambda;
    Type* lambda_type = NULL;
    Type* return_type = NULL;

    // What we need to do to infer the type of a function
    // We need it go inside, and resolve the individual arguments.
    // We also need to resolve all instances of return type.

    type_env = inferer_get_curr_type_env(inferer);
    inferer_decl_var_begin_inferrence(inferer, lambda.decl);

    return_type =
        inferer_convert_return_kind_to_default_type
            (inferer, lambda.return_type, lambda.decl->decl.var.return_kind);

    lambda_type = inferer_get_fn_type(inferer, lambda.argc, lambda.argv, return_type);
    inferer_decl_var_set_type(inferer, lambda.decl, lambda_type);

    if (!inferer_infer_stmt(inferer, lambda.body))
    {
        return false;
    }
    // I think this is not needed.
    // inferer_decl_var_generalize_inferred(inferer, fn.decl);
    inferer_set_curr_type_env(inferer, type_env);

    *type = lambda_type;
    return true;
}

void inferer_infer_expr_primary_decl(Inferer* inferer, Decl* decl, Type** type)
{
    assert(inferer != NULL);
    assert(decl    != NULL);
    assert(decl->kind == DECL_VAR);
    assert(type    != NULL);
    assert(*type   == NULL);

    TypeScheme* scheme = NULL;

    if (decl->decl.var.scheme != NULL)
    {
        scheme = inferer_decl_var_get_scheme(inferer, decl);
        assert(scheme != NULL);
        inferer_instantiate(inferer, scheme, type);
        return;
    }

    if (decl->decl.var.type != NULL)
    {
        *type = inferer_decl_var_get_type(inferer, decl);
        return;
    }

    UNREACHABLE;
}

void inferer_infer_expr_primary_identifier(Inferer* inferer, char* identifier, Type** type)
{
    assert(inferer    != NULL);
    assert(type       != NULL);
    assert(identifier != NULL);
    assert(*type      == NULL);

    // Not entirely correct - there are uses where identifier doesn't get resolved (for example, in struct access).
    // That use case should be handled elsewhere.
    UNREACHABLE;
}

bool inferer_infer_expr_primary(Inferer* inferer, Expr* expr, Type** type)
{
    assert(inferer != NULL);
    assert(expr    != NULL);
    assert(expr->kind == EXPR_PRIMARY);
    assert(type    != NULL);
    assert(*type   == NULL);

    ExprPrimary primary = expr->expr.primary;

    switch (primary.kind)
    {
        case EXPR_PRIMARY_UNKNOWN   : UNREACHABLE;

        // Primitives
        case EXPR_PRIMARY_NIL       : *type = builtin_type_nil   ; return true;
        case EXPR_PRIMARY_BOOLEAN   : *type = builtin_type_bool  ; return true;
        case EXPR_PRIMARY_STRING    : *type = builtin_type_string; return true;
        case EXPR_PRIMARY_NATURAL   : *type = builtin_type_nat   ; return true;
        case EXPR_PRIMARY_INTEGER   : *type = builtin_type_int   ; return true;
        case EXPR_PRIMARY_REAL      : *type = builtin_type_real  ; return true;

        // Derivative
        case EXPR_PRIMARY_LIST      : return inferer_infer_expr_primary_list   (inferer, expr, type);
        case EXPR_PRIMARY_STRUCT    : return inferer_infer_expr_primary_struct (inferer, expr, type);
        case EXPR_PRIMARY_LAMBDA    : return inferer_infer_expr_primary_lambda (inferer, expr, type);

        // Special
        case EXPR_PRIMARY_DECL      : inferer_infer_expr_primary_decl       (inferer, primary.primary.decl      , type); return true;
        case EXPR_PRIMARY_IDENTIFIER: inferer_infer_expr_primary_identifier (inferer, primary.primary.identifier, type); return true;
    }
    UNREACHABLE;
}

bool inferer_infer_expr_unary_logical_operator(Inferer* inferer, Expr* expr, Type** type)
{
    assert(inferer     != NULL);
    assert(expr    != NULL);
    assert(expr->kind == EXPR_UNARY);
    assert(type        != NULL);
    assert(*type       == NULL);

    ExprUnary unary  = expr->expr.unary;
    Type* unary_type = NULL;

    if (!inferer_infer_expr(inferer, unary.unary, &unary_type))
    {
        return false;
    }

    if (!inferer_unify(inferer, &unary_type, &builtin_type_bool, inferer_get_expr_span(inferer, expr)))
    {
        return false;
    }

    return true;
}

bool inferer_infer_expr_unary_arithmetic_operator(Inferer* inferer, Expr* expr, Type** type)
{
    assert(inferer     != NULL);
    assert(expr    != NULL);
    assert(expr->kind == EXPR_UNARY);
    assert(type        != NULL);
    assert(*type       == NULL);

    ExprUnary unary  = expr->expr.unary;
    Type* unary_type = NULL;

    if (!inferer_infer_expr(inferer, unary.unary, &unary_type))
    {
        return false;
    }

    if (!inferer_constrain_numeric(inferer, &unary_type, inferer_get_expr_span(inferer, expr)))
    {
        return false;
    }

    return true;
}

bool inferer_infer_expr_unary(Inferer* inferer, Expr* expr, Type** type)
{
    assert(inferer != NULL);
    assert(expr    != NULL);
    assert(expr->kind == EXPR_UNARY);
    assert(type        != NULL);
    assert(*type       == NULL);

    ExprUnary unary = expr->expr.unary;

    switch (unary.kind)
    {
        case EXPR_UNARY_UNKNOWN: UNREACHABLE;
        case EXPR_UNARY_NOT    : return inferer_infer_expr_unary_logical_operator   (inferer, expr, type);
        case EXPR_UNARY_NEGATE : return inferer_infer_expr_unary_arithmetic_operator(inferer, expr, type);
    }
    UNREACHABLE;
}

bool inferer_infer_expr_binary_arithmetic_operator(Inferer* inferer, Expr* expr, Type** type)
{
    assert(inferer != NULL);
    assert(expr    != NULL);
    assert(expr->kind == EXPR_BINARY);
    assert(type         != NULL);
    assert(*type        == NULL);

    Type* left  = NULL;
    Type* right = NULL;

    if (!inferer_infer_expr(inferer, expr->expr.binary.left , &left ))
    {
        return false;
    }

    if (!inferer_infer_expr(inferer, expr->expr.binary.right, &right))
    {
        return false;
    }

    if (!inferer_unify(inferer, &left, &right, inferer_get_expr_span(inferer, expr)))
    {
        return false;
    }

    // If unification is successful, then left ~ right.
    if (!inferer_constrain_numeric(inferer, &left, inferer_get_expr_span(inferer, expr)))
    {
        return false;
    }

    *type = left;
    return true;
}

bool inferer_infer_expr_binary_comparison_operator(Inferer* inferer, Expr* expr, Type** type)
{
    assert(inferer != NULL);
    assert(expr    != NULL);
    assert(expr->kind == EXPR_BINARY);
    assert(type         != NULL);
    assert(*type        == NULL);

    Type* left  = NULL;
    Type* right = NULL;

    if (!inferer_infer_expr(inferer, expr->expr.binary.left , &left ))
    {
        return false;
    }

    if (!inferer_infer_expr(inferer, expr->expr.binary.right, &right))
    {
        return false;
    }

    if (!inferer_unify(inferer, &left, &right, inferer_get_expr_span(inferer, expr)))
    {
        return false;
    }

    // If unification is successful, then left ~ right.
    if (!inferer_constrain_numeric(inferer, &left, inferer_get_expr_span(inferer, expr)))
    {
        return false;
    }

    *type = builtin_type_bool;
    return true;
}

bool inferer_infer_expr_binary_logical_operator(Inferer* inferer, Expr* expr, Type** type)
{
    assert(inferer != NULL);
    assert(expr    != NULL);
    assert(expr->kind == EXPR_BINARY);
    assert(type         != NULL);
    assert(*type        == NULL);

    Type* left  = NULL;
    Type* right = NULL;

    if (!inferer_infer_expr(inferer, expr->expr.binary.left , &left ))
    {
        return false;
    }

    if (!inferer_infer_expr(inferer, expr->expr.binary.right, &right))
    {
        return false;
    }

    if (!inferer_unify(inferer, &left, &right, inferer_get_expr_span(inferer, expr)))
    {
        return false;
    }

    // If unification is successful, then left ~ right.
    if (!inferer_unify(inferer, &left, &builtin_type_bool, inferer_get_expr_span(inferer, expr)))
    {
        return false;
    }

    return true;
}

bool inferer_infer_expr_binary_equality_operator(Inferer* inferer, Expr* expr, Type** type)
{
    assert(inferer != NULL);
    assert(expr    != NULL);
    assert(expr->kind == EXPR_BINARY);
    assert(type         != NULL);
    assert(*type        == NULL);

    Type* left  = NULL;
    Type* right = NULL;

    if (!inferer_infer_expr(inferer, expr->expr.binary.left , &left ))
    {
        return false;
    }

    if (!inferer_infer_expr(inferer, expr->expr.binary.right, &right))
    {
        return false;
    }

    if (!inferer_unify(inferer, &left, &right, inferer_get_expr_span(inferer, expr)))
    {
        return false;
    }

    // If unification is successful, then left ~ right.
    if (!inferer_constrain_equality(inferer, &left, inferer_get_expr_span(inferer, expr)))
    {
        return false;
    }

    *type = builtin_type_bool;
    return true;
}

bool inferer_infer_expr_binary_access_operator(Inferer* inferer, Expr* expr, Type** type)
{
    assert(inferer != NULL);
    assert(expr    != NULL);
    assert(expr->kind == EXPR_BINARY);
    assert(type    != NULL);
    assert(*type   == NULL);
    assert(expr->expr.binary.right->kind == EXPR_PRIMARY);
    assert(expr->expr.binary.right->expr.primary.kind == EXPR_PRIMARY_IDENTIFIER);

    ExprBinary binary = expr->expr.binary;
    bool is_successful = false;
    Type* left_return_type  = NULL;
    TypeStructField* right_field = NULL;

    is_successful = inferer_infer_expr(inferer, binary.left, &left_return_type);
    if (!is_successful)
    {
        return false;
    }

    if (left_return_type->kind != TYPE_STRUCT)
    {
        inferer_throw_err_expr_binary_access_op_left_kind_not_struct(inferer, expr);
        return false;
    }

    right_field = type_struct_find_key(left_return_type->type.structt, binary.right->expr.primary.primary.identifier); 
    if (right_field == NULL)
    {
        inferer_throw_err_expr_binary_access_op_struct_does_not_contain_field(inferer, expr);
        return false;
    }

    *type = right_field->value;
    return true;
}

bool inferer_infer_expr_binary_assign_operator(Inferer* inferer, Expr* expr, Type** type)
{
    assert(inferer != NULL);
    assert(expr    != NULL);
    assert(expr->kind == EXPR_BINARY);
    assert(type         != NULL);
    assert(*type        == NULL);

    Type* left  = NULL;
    Type* right = NULL;

    // TODO: Currently, we just infer both sides, and then try to unify them.
    // This does not seem entirely correct to me, though I haven't though about it enough to know why.
    // Therefore, this should probably be looked into.
    if (!inferer_infer_expr(inferer, expr->expr.binary.left , &left ))
    {
        return false;
    }

    if (!inferer_infer_expr(inferer, expr->expr.binary.right, &right))
    {
        return false;
    }

    if (!inferer_unify(inferer, &left, &right, inferer_get_expr_span(inferer, expr)))
    {
        return false;
    }

    *type = left;
    return true;
}

bool inferer_infer_expr_binary_index_operator(Inferer* inferer, Expr* expr, Type** type) 
{
    assert(inferer != NULL);
    assert(expr    != NULL);
    assert(expr->kind == EXPR_BINARY);
    assert(type         != NULL);
    assert(*type        == NULL);

    Type* left  = NULL;
    Type* right = NULL;
    Type* list  = NULL;

    if (!inferer_infer_expr(inferer, expr->expr.binary.left , &left ))
    {
        return false;
    }

    if (!inferer_infer_expr(inferer, expr->expr.binary.right, &right))
    {
        return false;
    }

    list = inferer_create_free_list_type(inferer);
    if (!inferer_unify(inferer, &left, &list, inferer_get_expr_span(inferer, expr)))
    {
        return false;
    }
    assert(list->kind == TYPE_LIST);

    if (!inferer_unify(inferer, &right, &builtin_type_int, inferer_get_expr_span(inferer, expr)))
    {
        return false;
    }

    *type = list->type.list.type;
    return true;
}

// TODO: Rethink think how to resolve struct access operator.
bool inferer_infer_expr_binary(Inferer* inferer, Expr* expr, Type** type)
{
    assert(inferer != NULL);
    assert(expr    != NULL);
    assert(expr->kind == EXPR_BINARY);
    assert(type    != NULL);
    assert(*type   == NULL);

    ExprBinary binary = expr->expr.binary;

    switch (binary.kind)
    {
        case EXPR_BINARY_UNKNOWN      : UNREACHABLE;

        // Arithmetic operators
        case EXPR_BINARY_ADD          : return inferer_infer_expr_binary_arithmetic_operator(inferer, expr, type);
        case EXPR_BINARY_SUBTRACT     : return inferer_infer_expr_binary_arithmetic_operator(inferer, expr, type);
        case EXPR_BINARY_MULTIPLY     : return inferer_infer_expr_binary_arithmetic_operator(inferer, expr, type);
        case EXPR_BINARY_DIVIDE       : return inferer_infer_expr_binary_arithmetic_operator(inferer, expr, type);
        case EXPR_BINARY_MODULO       : return inferer_infer_expr_binary_arithmetic_operator(inferer, expr, type);

        // Logical operators
        case EXPR_BINARY_AND          : return inferer_infer_expr_binary_logical_operator   (inferer, expr, type);
        case EXPR_BINARY_OR           : return inferer_infer_expr_binary_logical_operator   (inferer, expr, type);

        // Equality
        case EXPR_BINARY_EQUAL        : return inferer_infer_expr_binary_equality_operator  (inferer, expr, type);
        case EXPR_BINARY_NOT_EQUAL    : return inferer_infer_expr_binary_equality_operator  (inferer, expr, type);

        // Comparison
        // for now, comparisons are resolved similiarly to arithmetic expressions.
        case EXPR_BINARY_LESS_EQUAL   : return inferer_infer_expr_binary_comparison_operator(inferer, expr, type);
        case EXPR_BINARY_LESS         : return inferer_infer_expr_binary_comparison_operator(inferer, expr, type);
        case EXPR_BINARY_GREATER_EQUAL: return inferer_infer_expr_binary_comparison_operator(inferer, expr, type);
        case EXPR_BINARY_GREATER      : return inferer_infer_expr_binary_comparison_operator(inferer, expr, type);
        case EXPR_BINARY_CHAIN        : UNREACHABLE; // Shouldn't be encountered ideally
        case EXPR_BINARY_ACCESS       : return inferer_infer_expr_binary_access_operator    (inferer, expr, type);
        case EXPR_BINARY_ASSIGN       : return inferer_infer_expr_binary_assign_operator    (inferer, expr, type);
        case EXPR_BINARY_INDEX        : return inferer_infer_expr_binary_index_operator     (inferer, expr, type);
    }
    UNREACHABLE;
}

bool inferer_infer_expr_fn(Inferer* inferer, Expr* expr, Type** type)
{
    assert(inferer != NULL);
    assert(expr    != NULL);
    assert(expr->kind == EXPR_FN);
    assert(type    != NULL);
    assert(*type   == NULL);

    ExprFn fn = expr->expr.fn ;

    Type* caller_type = NULL;
    Type* curr_caller_branch_type = NULL;

    if (!inferer_infer_expr(inferer, fn.caller, &caller_type))
    {
        return false;
    }
    curr_caller_branch_type = caller_type;

    for (int i = 0; i < fn.argc; ++i)
    {
        assert(curr_caller_branch_type != NULL);
        if (curr_caller_branch_type->kind != TYPE_FN)
        {
            inferer_throw_err_expr_fn_excessive_args(inferer, expr);
            return false;
        }

        Type*   curr_expr_arg_type = NULL;
        Type* curr_caller_arg_type = curr_caller_branch_type->type.fn.left;

        if (!inferer_infer_expr(inferer, fn.argv[i], &curr_expr_arg_type))
        {
            return false;
        }

        if (!inferer_unify
            (inferer, &curr_caller_arg_type, &curr_expr_arg_type,
                inferer_get_expr_span(inferer, expr)))
        {
            return false;
        }

        curr_caller_branch_type = curr_caller_branch_type->type.fn.right;
    }

    *type = curr_caller_branch_type;
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
        case EXPR_PRIMARY: return inferer_infer_expr_primary(inferer, expr, type);
        case EXPR_UNARY  : return inferer_infer_expr_unary  (inferer, expr, type);
        case EXPR_BINARY : return inferer_infer_expr_binary (inferer, expr, type);
        case EXPR_FN     : return inferer_infer_expr_fn     (inferer, expr, type);
    }
    UNREACHABLE;
}

void inferer_convert_type_expr_variable(Inferer* inferer, TypeExprVariable variable, Type** type)
{
    assert(inferer   != NULL);
    assert(type      != NULL);
    assert(*type     == NULL);

    *type = inferer_decl_type_var_get_type(inferer, variable.decl);
    if (*type == NULL)
    {
        *type = inferer_create_free_type_var(inferer);
        inferer_decl_type_var_set_type(inferer, variable.decl, *type);
    }
}

void inferer_convert_type_expr_list(Inferer* inferer, TypeExprList list, Type** type)
{
    assert(inferer   != NULL);
    assert(type      != NULL);
    assert(*type     == NULL);

    Type  type_mem;
    Type* sub_type = NULL;

    inferer_convert_type_expr(inferer, list.type, &sub_type);

    type_mem = (Type)
    {
        .kind = TYPE_LIST,
        .type.list = (TypeList) { .type = sub_type },
    };
    *type = (Type*) arena_push(&inferer->type_arena, &type_mem, sizeof(Type));
}

void inferer_convert_type_expr_struct(Inferer* inferer, TypeExprStruct structt, Type** type)
{
    assert(inferer   != NULL);
    assert(type      != NULL);
    assert(*type     == NULL);


    Type type_mem;
    int field_num = 0;
    DYNAMIC_ARRAY(TypeStructField** fields) = NULL;


    for (int i = 0; i < structt.argc; ++i)
    {
        TypeExprStructField* curr_field = structt.argv[i];
        TypeStructField  curr_type_field;
        TypeStructField* pushed_field = NULL;
        Type* recv_type = NULL;


        curr_type_field.key = curr_field->key;
        inferer_convert_type_expr(inferer, curr_field->value, &recv_type);
        curr_type_field.value = recv_type;

        pushed_field = (TypeStructField*) arena_push(&inferer->type_arena, &curr_type_field, sizeof(TypeStructField));
        arrput(fields, pushed_field);
    }

    TypeStructField** tmp_ptr = fields;
    field_num = arrlen(tmp_ptr);
    fields = (TypeStructField**) arena_push(&inferer->type_arena, tmp_ptr, field_num * sizeof(TypeStructField*));
    arrfree(tmp_ptr);

    type_mem = (Type)
    {
        .kind = TYPE_STRUCT,
        .type.structt = (TypeStruct)
        {
            .field_num = field_num,
            .fields    = fields   ,
        }
    };

    *type = (Type*) arena_push(&inferer->type_arena, &type_mem, sizeof(Type));
}

void inferer_convert_type_expr_fn(Inferer* inferer, TypeExprFn fn, Type** type)
{
    assert(inferer   != NULL);
    assert(type      != NULL);
    assert(*type     == NULL);

    Type  type_mem;
    Type* left_type  = NULL;
    Type* right_type = NULL;

    inferer_convert_type_expr(inferer, fn.left , &left_type );
    inferer_convert_type_expr(inferer, fn.right, &right_type);

    type_mem = (Type)
    {
        .kind = TYPE_FN,
        .type.fn = (TypeFn)
        {
            .left  =  left_type,
            .right = right_type,
        },
    };
    *type = (Type*) arena_push(&inferer->type_arena, &type_mem, sizeof(Type));
}

void inferer_convert_type_expr_application(Inferer* inferer, TypeExprApplication application, Type** type)
{
    assert(inferer   != NULL);
    assert(type      != NULL);
    assert(*type     == NULL);

    Type type_mem;
    TypeAbstraction* abstraction;
    DYNAMIC_ARRAY(Type** argv) = NULL;
    int argc = 0;

    abstraction = inferer_get_existing_new_type_from_decl(inferer, application.decl);

    for (int i = 0; i < application.argc; ++i)
    {
        TypeExpr* te = application.argv[i];
        Type*     t  = NULL;

        inferer_convert_type_expr(inferer, te, &t);

        arrput(argv, t);
    }

    Type** tmp_ptr = argv;
    argc = arrlen(tmp_ptr);
    argv = (Type**) arena_push(&inferer->type_arena, argv, argc * sizeof(Type*));
    arrfree(tmp_ptr);

    type_mem = (Type)
    {
        .kind = TYPE_APPLICATION,
        .type.application = (TypeApplication)
        {
            .abstraction = abstraction,
            .argv = argv,
            .argc = argc,
        }
    };

    *type = (Type*) arena_push(&inferer->type_arena, &type_mem, sizeof(Type*));
}

void inferer_conver_type_expr_alias(Inferer* inferer, TypeExprAlias alias, Type** type)
{
    assert(inferer   != NULL);
    assert(type      != NULL);
    assert(*type     == NULL);

    *type = inferer_decl_alias_get_type(inferer, alias.decl);
    assert(*type != NULL); // Shouldn't really ever be NULL.
}

void inferer_convert_type_expr(Inferer* inferer, TypeExpr* type_expr, Type** type)
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
        case TYPE_EXPR_VARIABLE   :
            inferer_convert_type_expr_variable
                (inferer, type_expr->type_expr.variable, type); return;

        case TYPE_EXPR_IDENTIFIER : UNREACHABLE; // Shouldn't be encountered at this stage.

        case TYPE_EXPR_NIL        : *type = builtin_type_nil   ; return;
        case TYPE_EXPR_BOOL       : *type = builtin_type_bool  ; return;
        case TYPE_EXPR_NAT        : *type = builtin_type_nat   ; return;
        case TYPE_EXPR_INT        : *type = builtin_type_int   ; return;
        case TYPE_EXPR_REAL       : *type = builtin_type_real  ; return;
        case TYPE_EXPR_STRING     : *type = builtin_type_string; return;

        case TYPE_EXPR_LIST       :
            inferer_convert_type_expr_list
                (inferer, type_expr->type_expr.list   , type); return;

        case TYPE_EXPR_STRUCT     :
            inferer_convert_type_expr_struct
                (inferer, type_expr->type_expr.structt, type); return;

        case TYPE_EXPR_FN         :
            inferer_convert_type_expr_fn
                (inferer, type_expr->type_expr.fn     , type); return;

        case TYPE_EXPR_INSTANCE   : UNREACHABLE;

        case TYPE_EXPR_APPLICATION:
            inferer_convert_type_expr_application
                (inferer, type_expr->type_expr.application, type); return;

        case TYPE_EXPR_ALIAS      :
            inferer_conver_type_expr_alias
                (inferer, type_expr->type_expr.alias, type); return;
    }
    UNREACHABLE;
}

bool inferer_infer_stmt_block(Inferer* inferer, StmtBlock block)
{
    assert(inferer    != NULL);

    for (int i = 0; i < block.size; ++i)
    {
        Stmt* inner_stmt = block.body[i];
        if (!inferer_infer_stmt(inferer, inner_stmt))
        {
            return false;
        }
    }

    return true;
}

bool inferer_infer_stmt_while(Inferer* inferer, StmtWhile whilee)
{
    assert(inferer    != NULL);

    Type* type = NULL;

    if (!inferer_infer_expr(inferer, whilee.condition, &type))
    {
        return false;
    }

    if (!inferer_infer_stmt(inferer, whilee.body))
    {
        return false;
    }

    return true;
}

bool inferer_infer_stmt_if(Inferer* inferer, StmtIf iff)
{
    assert(inferer    != NULL);

    Type* type = NULL;

    if (!inferer_infer_expr(inferer, iff.condition, &type))
    {
        return false;
    }

    if (!inferer_infer_stmt(inferer, iff.body))
    {
        return false;
    }

    return true;
}

bool inferer_infer_stmt_return(Inferer* inferer, StmtReturn returnn)
{
    assert(inferer    != NULL);

    Type* expr_type    = NULL ;
    Type* return_type  = NULL ;

    if (returnn.expr == NULL)
    {
        expr_type = builtin_type_nil;
    }
    else if (returnn.expr != NULL && !inferer_infer_expr(inferer, returnn.expr, &expr_type))
    {
        return false;
    }

    return_type = inferer_decl_var_get_return_type(inferer, returnn.decl);

    if (!inferer_unify
        (inferer, &return_type, &expr_type, inferer_get_expr_span(inferer, returnn.expr)))
    {
        return false;
    }

    inferer_decl_var_set_return_type(inferer, returnn.decl, return_type);

    return true;
}

bool inferer_infer_stmt_let(Inferer* inferer, StmtLet let)
{
    assert(inferer  != NULL);
    assert(let.expr != NULL);

    Type* expr_type       = NULL;
    Type* annotation_type = NULL;
    Type* final_type      = NULL;
    TypeEnv type_env;

    type_env = inferer_get_curr_type_env(inferer);
    inferer_decl_var_begin_inferrence(inferer, let.decl);

    if (!inferer_infer_expr(inferer, let.expr, &expr_type))
    {
        return false;
    }

    if (let.type != NULL)
    {
        inferer_convert_type_expr(inferer, let.type, &annotation_type);

        inferer_unify(inferer, &expr_type, &annotation_type, inferer_get_expr_span(inferer, let.expr));

        final_type = annotation_type;
    }
    else
    {
        final_type = expr_type;
    }

    inferer_decl_var_set_type(inferer, let.decl, final_type);
    inferer_decl_var_generalize_inferred(inferer, let.decl);
    inferer_set_curr_type_env(inferer, type_env);

    return true;
}

bool inferer_infer_stmt_fn(Inferer* inferer, StmtFn fn)
{
    assert(inferer != NULL);

    TypeEnv type_env;
    Type* fn_type     = NULL;
    Type* return_type = NULL;

    // What we need to do to infer the type of a function
    // We need it go inside, and resolve the individual arguments.
    // We also need to resolve all instances of return type.

    type_env = inferer_get_curr_type_env(inferer);
    inferer_decl_var_begin_inferrence(inferer, fn.decl);

    return_type =
        inferer_convert_return_kind_to_default_type
            (inferer, fn.return_type, fn.decl->decl.var.return_kind);

    fn_type = inferer_get_fn_type(inferer, fn.argc, fn.argv, return_type);
    inferer_decl_var_set_type(inferer, fn.decl, fn_type);

    if (!inferer_infer_stmt(inferer, fn.body))
    {
        return false;
    }
    inferer_decl_var_generalize_inferred(inferer, fn.decl);
    inferer_set_curr_type_env(inferer, type_env);

    return true;
}

bool inferer_infer_stmt_alias(Inferer* inferer, StmtAlias alias)
{
    assert(inferer != NULL);

    Type* type = NULL;

    inferer_convert_type_expr(inferer, alias.type, &type);

    inferer_decl_alias_set_type(inferer, alias.decl, type);

    return true;
}

bool inferer_infer_stmt_type(Inferer* inferer, StmtType stmt_type)
{
    assert(inferer != NULL);

    // IMPLEMENT:
    UNREACHABLE;

    // struct TypeApplication
    // {
    //     TypeAbstraction* abstraction;
    //     Type** argv;
    //     int    argc;
    //     // should be the same as the lenght of abstraction->type.abstraction.argc
    // };

    // struct DeclType
    // {
    //     TypeAbstraction* abstraction;
    //     Decl** type_vars;
    //     Decl** constructors;
    //     int type_var_num;
    //     int constructor_num;
    // };

    // struct DeclTypeConstructor
    // {
    //     TypeExpr** types;
    //     int type_num;
    // };

    Type* type = NULL;
    TypeAbstraction* abstraction = NULL;

    abstraction = inferer_create_type_abstraction(inferer, stmt_type.decl);
    inferer_decl_type_set_abstraction(inferer, stmt_type.decl, abstraction);
}

bool inferer_infer_stmt_match(Inferer* inferer, StmtMatch match)
{
    assert(inferer != NULL);

    // IMPLEMENT:
    UNREACHABLE;
}

bool inferer_infer_stmt(Inferer* inferer, Stmt* stmt)
{
    assert(inferer != NULL);
    assert(stmt    != NULL);

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
        case STMT_ALIAS   : return inferer_infer_stmt_alias (inferer, stmt->stmt.alias  );
        case STMT_TYPE    : return inferer_infer_stmt_type  (inferer, stmt->stmt.type   );
        case STMT_MATCH   : return inferer_infer_stmt_match (inferer, stmt->stmt.match  );
    }
    UNREACHABLE;
}

Type* inferer_create_free_type_var(Inferer* inferer)
{
    assert(inferer != NULL);

    Type type_mem;
    Type* type = NULL;

    type_mem = (Type) { .kind = TYPE_FREE_VAR, };

    type = (Type*) arena_push(&inferer->type_arena, &type_mem, sizeof(Type));
    inferer_push_type_variable(inferer, type);

    return type;
}

Type* inferer_create_free_list_type(Inferer* inferer)
{
    assert(inferer != NULL);

    Type type_mem;

    type_mem = (Type)
    {
        .kind = TYPE_LIST,
        .type.list = (TypeList) { .type = inferer_create_free_type_var(inferer) },
    };

    return (Type*) arena_push(&inferer->type_arena, &type_mem, sizeof(Type));
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

Type* inferer_create_list_type(Inferer* inferer, Type* inferred_type)
{
    assert(inferer != NULL);

    Type type_mem;

    type_mem = (Type)
    {
        .kind = TYPE_LIST,
        .type.list = (TypeList) { .type = inferred_type },
    };

    return (Type*) arena_push(&inferer->type_arena, &type_mem, sizeof(Type));
}

TypeAbstraction* inferer_create_type_abstraction(Inferer* inferer, int type_var_num)
{
    assert(inferer != NULL);
    assert(type_var_num >= 0);

    TypeAbstraction  abstraction_mem;
    TypeAbstraction* abstraction = NULL;

    abstraction_mem = (TypeAbstraction)
    {
        .argc = type_var_num,
    };

    abstraction = (TypeAbstraction*) arena_push(&inferer->type_arena, &abstraction_mem, sizeof(TypeAbstraction));
    return abstraction;
}

TypeAbstraction* inferer_get_existing_new_type_from_decl(Inferer* inferer, Decl* decl)
{
    assert(inferer != NULL);
    assert(decl    != NULL);
    assert(decl->kind == DECL_TYPE);

    if (decl->decl.type.abstraction == NULL)
    {
        decl->decl.type.abstraction = inferer_create_type_abstraction(inferer, decl->decl.type.type_var_num);
    }

    return decl->decl.type.abstraction;
}

void inferer_decl_var_begin_inferrence(Inferer* inferer, Decl* decl)
{
    assert(inferer != NULL);
    assert(decl    != NULL);

    inferer_decl_var_set_type(inferer, decl, inferer_create_free_type_var(inferer));
}

Type* inferer_decl_var_get_type(Inferer* inferer, Decl* decl)
{
    assert(inferer != NULL);
    assert(decl    != NULL);
    assert(decl->kind == DECL_VAR);
    assert(decl->decl.var.type != NULL);

    return decl->decl.var.type;
}

void inferer_decl_var_set_type(Inferer* inferer, Decl* decl, Type* type)
{
    assert(inferer != NULL);
    assert(decl    != NULL);
    assert(decl->kind == DECL_VAR);

    decl->decl.var.type = type;
}

Type* inferer_decl_var_get_return_type(Inferer* inferer, Decl* decl)
{
    assert(inferer != NULL);
    assert(decl    != NULL);
    assert(decl->kind == DECL_VAR);
    assert(decl->decl.var.type != NULL);
    assert(decl->decl.var.type->kind == TYPE_FN);

    Type* type = decl->decl.var.type;

    while (type->kind == TYPE_FN)
    {
        type = type->type.fn.right;
    }

    return type;
}

void inferer_decl_var_set_return_type(Inferer* inferer, Decl* decl, Type* type)
{
    assert(inferer != NULL);
    assert(decl    != NULL);
    assert(decl->kind == DECL_VAR);
    assert(decl->decl.var.type != NULL);
    assert(decl->decl.var.type->kind == TYPE_FN);

    Type* old_type = decl->decl.var.type;

    while (old_type->type.fn.right->kind == TYPE_FN)
    {
        old_type = old_type->type.fn.right;
    }

    old_type->type.fn.right = type;
}

Type* inferer_decl_type_var_get_type(Inferer* inferer, Decl* decl)
{
    assert(inferer != NULL);
    assert(decl    != NULL);
    assert(decl->kind == DECL_TYPE_VAR);

    return decl->decl.type_var.type;
}

void inferer_decl_type_var_set_type(Inferer* inferer, Decl* decl, Type* type)
{
    assert(inferer != NULL);
    assert(decl    != NULL);
    assert(decl->kind == DECL_TYPE_VAR);

    decl->decl.type_var.type = type;
}

Type* inferer_decl_alias_get_type(Inferer* inferer, Decl* decl)
{
    assert(inferer != NULL);
    assert(decl    != NULL);
    assert(decl->kind == DECL_ALIAS);

    return decl->decl.alias.type;
}

void inferer_decl_alias_set_type(Inferer* inferer, Decl* decl, Type* type)
{
    assert(inferer != NULL);
    assert(decl    != NULL);
    assert(decl->kind == DECL_ALIAS);

    decl->decl.alias.type = type;
}

void  inferer_decl_var_generalize_inferred (Inferer* inferer, Decl* decl)
{
    assert(inferer != NULL);
    assert(decl    != NULL);
    assert(decl->kind == DECL_VAR);
    assert(decl->decl.var.type != NULL);

    inferer_generalize(inferer, decl->decl.var.type, &decl->decl.var.scheme);
}

TypeScheme* inferer_decl_var_get_scheme(Inferer* inferer, Decl* decl)
{
    assert(inferer != NULL);
    assert(decl    != NULL);
    assert(decl->kind == DECL_VAR);
    assert(decl->decl.var.scheme != NULL);

    return decl->decl.var.scheme;
}

void inferer_decl_var_set_scheme(Inferer* inferer, Decl* decl, TypeScheme* scheme)
{
    assert(inferer != NULL);
    assert(decl    != NULL);
    assert(decl->kind == DECL_VAR);

    decl->decl.var.scheme = scheme;
}

TypeAbstraction* inferer_decl_type_get_abstraction(Inferer* inferer, Decl* decl)
{
    assert(inferer != NULL);
    assert(decl != NULL);
    assert(decl->kind == DECL_TYPE);

    return decl->decl.type.abstraction;
}

void  inferer_decl_type_set_abstraction(Inferer* inferer, Decl* decl, TypeAbstraction* abstraction)
{
    assert(inferer != NULL);
    assert(decl != NULL);
    assert(decl->kind == DECL_TYPE);

    decl->decl.type.abstraction = abstraction;
}

void inferer_push_type_variable(Inferer* inferer, Type* type_var)
{
    assert(inferer != NULL);

    arrput(inferer->type_variables, type_var);
    inferer->curr_type_env.type_variable_length++;
}

void inferer_pop_type_variable(Inferer* inferer)
{
    assert(inferer != NULL);

    (void) arrpop(inferer->type_variables);
    inferer->curr_type_env.type_variable_length--;
}

bool inferer_occurs_check_struct(Inferer* inferer, Type* var, TypeStruct structt)
{
    assert(inferer   != NULL         );
    assert(var       != NULL         );
    assert(var->kind == TYPE_FREE_VAR);

    for (int i = 0; i < structt.field_num; ++i)
    {
        TypeStructField* field = structt.fields[i];

        if (!inferer_occurs_check(inferer, var, field->value))
        {
            return false;
        }
    }

    return true;
}

bool inferer_occurs_check_application(Inferer* inferer, Type* var, TypeApplication application)
{
    assert(inferer   != NULL         );
    assert(var       != NULL         );
    assert(var->kind == TYPE_FREE_VAR);

    for (int i = 0; i < application.argc; ++i)
    {
        Type* type = application.argv[i];

        if (!inferer_occurs_check(inferer, var, type))
        {
            return false;
        }
    }

    return true;
}

// Nothing evil found - true, else - false;
bool inferer_occurs_check(Inferer* inferer, Type* var, Type* type)
{
    assert(inferer   != NULL         );
    assert(var       != NULL         );
    assert(type      != NULL         );
    assert(var->kind == TYPE_FREE_VAR);

    switch (type->kind)
    {
        case TYPE_NIL        : return true;
        case TYPE_BOOL       : return true;
        case TYPE_NAT        : return true;
        case TYPE_INT        : return true;
        case TYPE_REAL       : return true;
        case TYPE_STRING     : return true;
        case TYPE_LIST       : return inferer_occurs_check(inferer, var, type->type.list.type);
        case TYPE_STRUCT     : return inferer_occurs_check_struct(inferer, var, type->type.structt);
        case TYPE_FN         : return inferer_occurs_check(inferer, var, type->type.fn.left )
                                   && inferer_occurs_check(inferer, var, type->type.fn.right);
        case TYPE_FREE_VAR   : return var != type;
        case TYPE_BOUNDED_VAR: UNREACHABLE;

        case TYPE_APPLICATION: return inferer_occurs_check_application(inferer, var, type->type.application);
        case TYPE_CONSTRUCTOR: return inferer_occurs_check(inferer, var, type->type.fn.left   )
                                   && inferer_occurs_check(inferer, var, type->type.fn.right  );
        case TYPE_ALIAS      : return inferer_occurs_check(inferer, var, type->type.alias.type);
    }
    UNREACHABLE;
}

// TODO: Take a look at
bool inferer_bind_variable_to_type(Inferer* inferer, Type** var_ref, Type* type)
{
    assert(inferer   != NULL);
    assert(var_ref   != NULL);
    assert(*var_ref  != NULL);
    assert(type      != NULL);
    assert((*var_ref)->kind == TYPE_FREE_VAR);

    Bind  bind;
    Type* var = *var_ref;

    if (var == type)
    {
        return true;
    }

    if (!inferer_occurs_check(inferer, var, type))
    {
        return false;
    }

    bind = (Bind)
    {
        .var_ref = var_ref,
        .type    = type   ,
    };
    arrput(inferer->binds, bind);

    return true;
}

void inferer_unify_apply_binds(Inferer* inferer)
{
    assert(inferer != NULL);

    int binds_length = arrlen(inferer->binds);
    int decl_length  = arrlen(inferer->declarations);

    for (int i = 0; i < binds_length; ++i)
    {
        Bind* bind = inferer->binds + i;

        Subst subst = (Subst)
        {
            .key   = (*bind->var_ref),
            .value = bind->type,
        };

        *bind->var_ref = bind->type;
        for (int j = 0; j < decl_length; ++j)
        {
            Decl* decl = inferer->declarations[j];
            inferer_decl_apply_subst(inferer, decl, subst);
        }
    }
}

void inferer_unify_free_binds(Inferer* inferer)
{
    assert(inferer != NULL);

    arrfree(inferer->binds);
    inferer->binds = NULL  ;
}

TypeEnv inferer_get_curr_type_env(Inferer* inferer)
{
    assert(inferer != NULL);

    return inferer->curr_type_env;
}

void inferer_set_curr_type_env(Inferer* inferer, TypeEnv type_env)
{
    assert(inferer != NULL);
    assert(inferer->curr_type_env.type_variable_length - type_env.type_variable_length >= 0);

    TypeEnv curr_type_env = inferer->curr_type_env;

    for (int i = 0; i < curr_type_env.type_variable_length - type_env.type_variable_length; ++i)
    {
        inferer_pop_type_variable(inferer);
    }

    inferer->curr_type_env = type_env;
}

bool inferer_type_applications_are_equal(Inferer* inferer, TypeApplication left_application, TypeApplication right_application)
{
    assert(inferer != NULL);

    return left_application.argc == right_application.argc
        && left_application.abstraction == right_application.abstraction;
}

bool inferer_subst_is_free_in_type_env(Inferer* inferer, Subst subst)
{
    assert(inferer != NULL);

    Type** type_variables = inferer->type_variables;
    for (int i = 0; i < arrlen(type_variables); ++i)
    {
        Type* type_var = type_variables[i];
        assert(type_var->kind == TYPE_FREE_VAR);
        if (type_var == subst.key)
        {
            return true;
        }
    }

    return false;
}

Type* inferer_convert_return_kind_to_default_type(Inferer* inferer, TypeExpr* type_expr, DeclVarReturnKind return_kind)
{
    assert(inferer != NULL);

    switch (return_kind)
    {
        case DECL_VAR_RETURN_NONE    :
        case DECL_VAR_RETURN_NULL    :
            return builtin_type_nil;

        case DECL_VAR_RETURN_NOT_NULL:
            if (type_expr == NULL)
            {
                return inferer_create_free_type_var(inferer);
            }
            else
            {
                Type* return_type = NULL;
                inferer_convert_type_expr(inferer, type_expr, &return_type);
                return return_type;
            }
    }

    UNREACHABLE;
}

Type* inferer_get_fn_type(Inferer* inferer, int argc, FnArg** argv, Type* return_type)
{
    assert(inferer     != NULL);
    assert(argv        != NULL);
    assert(return_type != NULL);

    Type  type_mem;
    Type* curr_type = return_type;

    // Special case if the function has no arguments - the left argument is NULL.
    if (argc == 0)
    {
        type_mem = (Type)
        {
            .kind = TYPE_FN,
            .type.fn = { .left = NULL, .right = curr_type }
        };
        curr_type = arena_push(&inferer->type_arena, &type_mem, sizeof(Type));
    }

    for (int i = argc - 1; i >= 0; --i)
    {
        FnArg* arg   = argv[i];
        Type*  arg_type = NULL;

        // NOTE: We NEVER generalize each the declarations of the function arguments.
        inferer_decl_var_begin_inferrence(inferer, arg->decl);

        if (arg->type != NULL)
        {
            inferer_convert_type_expr(inferer, arg->type, &arg_type);
        }
        else
        {
            arg_type = inferer_create_free_type_var(inferer);
        }

        inferer_decl_var_set_type(inferer, arg->decl, arg_type);

        type_mem = (Type)
        {
            .kind = TYPE_FN,
            .type.fn = { .left = arg_type, .right = curr_type }
        };

        curr_type = arena_push(&inferer->type_arena, &type_mem, sizeof(Type));
    }

    return curr_type;
}

Span inferer_get_expr_span(Inferer* inferer, Expr* expr)
{
    assert(inferer != NULL);
    assert(expr != NULL);

    return (Span)
    {
        .filename = inferer->filename,
        .line     = expr->line       ,
        .column   = expr->column     ,
        .length   = expr->length     ,
    };
}

void inferer_throw_err_unify_failed(Inferer* inferer, Span span, Type* left, Type* right)
{
    assert(inferer != NULL);
    assert(left    != NULL);
    assert(right   != NULL);

    diagnostic_component_push_err(inferer->diagnostic_component, (DiagnosticErr)
    {
        .kind = DIAGNOSTIC_ERR_UNIFY_FAILED,
        .span = span,
        .err.unify_failed =
        {
            .left  = left,
            .right = right,
        }
    });
}

void inferer_throw_err_type_failed_constraint_numeric(Inferer* inferer, Span span)
{
    assert(inferer != NULL);

    diagnostic_component_push_err(inferer->diagnostic_component, (DiagnosticErr)
    {
        .kind = DIAGNOSTIC_ERR_TYPE_FAILED_CONSTRAINT_NUMERIC,
        .span = span,
    });
}

void inferer_throw_err_type_failed_constraint_equality(Inferer* inferer, Span span)
{
    assert(inferer != NULL);

    diagnostic_component_push_err(inferer->diagnostic_component, (DiagnosticErr)
    {
        .kind = DIAGNOSTIC_ERR_TYPE_FAILED_CONSTRAINT_EQUALITY,
        .span = span,
    });
}

void inferer_throw_err_expr_binary_arithmetic_constraint_failed(Inferer* inferer, Expr* expr)
{
    assert(inferer != NULL);

    diagnostic_component_push_err(inferer->diagnostic_component, (DiagnosticErr)
    {
        .kind = DIAGNOSTIC_ERR_EXPR_BINARY_ARITHMETIC_CONSTRAINT_FAILED,
        .span = inferer_get_expr_span(inferer, expr),
    });
}

void inferer_throw_err_expr_binary_equality_constraint_failed(Inferer* inferer, Expr* expr)
{
    assert(inferer != NULL);

    diagnostic_component_push_err(inferer->diagnostic_component, (DiagnosticErr)
    {
        .kind = DIAGNOSTIC_ERR_EXPR_BINARY_EQUALITY_CONSTRAINT_FAILED,
        .span = inferer_get_expr_span(inferer, expr),
    });
}

void inferer_throw_err_expr_binary_access_op_left_kind_not_struct(Inferer* inferer, Expr* expr)
{
    assert(inferer != NULL);

    diagnostic_component_push_err(inferer->diagnostic_component, (DiagnosticErr)
    {
        .kind = DIAGNOSTIC_ERR_EXPR_BINARY_ACCESS_OP_LEFT_KIND_NOT_STRUCT,
        .span = inferer_get_expr_span(inferer, expr),
    });
}

void inferer_throw_err_expr_binary_access_op_struct_does_not_contain_field( Inferer* inferer, Expr* expr)
{
    assert(inferer != NULL);

    diagnostic_component_push_err(inferer->diagnostic_component, (DiagnosticErr)
    {
        .kind = DIAGNOSTIC_ERR_EXPR_BINARY_ACCESS_OP_STRUCT_DOES_NOT_CONTAIN_FIELD,
        .span = inferer_get_expr_span(inferer, expr),
    });
}

void inferer_throw_err_expr_fn_excessive_args(Inferer* inferer, Expr* expr)
{
    assert(inferer != NULL);

    diagnostic_component_push_err(inferer->diagnostic_component, (DiagnosticErr)
    {
        .kind = DIAGNOSTIC_ERR_EXPR_FN_EXCESSIVE_ARGS,
        .span = inferer_get_expr_span(inferer, expr),
    });
}

void inferer_throw_err_type_isnt_numeric(Inferer* inferer, Span span)
{
    assert(inferer != NULL);

    diagnostic_component_push_err(inferer->diagnostic_component, (DiagnosticErr)
    {
        .kind = DIAGNOSTIC_TYPE_ISNT_NUMERIC,
        .span = span,
    });
}

void inferer_throw_err_type_isnt_equality(Inferer* inferer, Span span)
{
    assert(inferer != NULL);

    diagnostic_component_push_err(inferer->diagnostic_component, (DiagnosticErr)
    {
        .kind = DIAGNOSTIC_TYPE_ISNT_EQUALITY,
        .span = span,
    });
}
