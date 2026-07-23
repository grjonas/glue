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
        .binds          = NULL,
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
        // .inside_function  = false,
        .curr_fn          = NULL ,
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

    arrfree(inferer->binds);

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
        .errs           = NULL,
    };
}

bool inferer_attempt_unify_left_type_numeric_helper(Inferer* inferer, TypeKind left_kind, Type** right_ref)
{
    assert(inferer    != NULL);
    assert(type_kind_is_numeric(left_kind));
    assert(right_ref  != NULL);
    assert(*right_ref != NULL);

    Type* right = *right_ref;

    if (right->kind == left_kind)
    {
        return true;
    }
    else if (right->kind == TYPE_NUMERIC)
    {
        right->kind = left_kind;
        return true;
    }
    else
    {
        return false;
    }
}

bool inferer_attempt_unify_left_type_numeric(Inferer* inferer, Type** left_ref, Type** right_ref)
{
    assert(inferer    != NULL);
    assert(left_ref   != NULL);
    assert(right_ref  != NULL);
    assert(*left_ref  != NULL);
    assert(*right_ref != NULL);

    Type* left  = *left_ref ;
    Type* right = *right_ref;

    assert(type_kind_is_numeric(left->kind));

    if (!type_kind_is_numeric(right->kind))
    {
        return false;
    }

    if (left->kind == right->kind)
    {
        return true;
    }

    switch (left->kind)
    {
        case TYPE_NAT    :
        case TYPE_INT    :
        case TYPE_REAL   :
             return inferer_attempt_unify_left_type_numeric_helper(inferer, left->kind, right_ref);

        // Swap
        case TYPE_NUMERIC: return inferer_attempt_unify_left_type_numeric(inferer, right_ref, left_ref);
        default:
            assert(false);
    }

}

bool inferer_attempt_unify_left_type_list(Inferer* inferer, TypeList left_list, Type** right_ref)
{
    assert(inferer    != NULL);
    assert(right_ref  != NULL);
    assert(*right_ref != NULL);

    bool is_successful = false;
    Type* right = *right_ref;

    if (right->kind == TYPE_LIST)
    {
        is_successful = inferer_attempt_unify(inferer, &left_list.type, &right->type.list.type);
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

bool inferer_attempt_unify_left_type_struct(Inferer* inferer, TypeStruct left_struct, Type** right_ref)
{
    assert(inferer    != NULL);
    assert(right_ref  != NULL);
    assert(*right_ref != NULL);

    bool is_successful = false;
    Type* right = *right_ref;
    TypeStruct right_struct;

    assert(false);

    if (right->kind != TYPE_STRUCT)
    {
        return false;
    }
    right_struct = right->type.structt;

    // TODO: This (the rest of the function) could be made more efficient.
    // TODO: Make sure duplicate identifiers are detected during both type_expr and expr struct field parsing.
    for (int i = 0; i < left_struct.field_num; ++i)
    {
        TypeStructField* left_field  = left_struct.fields[i];
        TypeStructField* right_field = type_struct_find_key(right_struct, left_field->key); 

        if (right_field == NULL)
        {
            return false;
        }

        is_successful = inferer_attempt_unify(inferer, &left_field->value, &right_field->value);
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

bool inferer_attempt_unify_left_type_fn(Inferer* inferer, TypeFn left_fn, Type** right_ref)
{
    assert(inferer    != NULL);
    assert(right_ref  != NULL);
    assert(*right_ref != NULL);

    bool is_successful = false;
    Type* right = *right_ref;

    if (right->kind == TYPE_FN)
    {
        is_successful = inferer_attempt_unify(inferer, &left_fn.left, &right->type.fn.left);
        if (!is_successful)
        {
            return false;
        }

        is_successful = inferer_attempt_unify(inferer, &left_fn.right, &right->type.fn.right);
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

// Attempts to attempt_unify two types
// Returns:
//     * 'true'  on successful unification,
//     makes changes to both the left and right types.
//     * 'false' on failure to attempt_unify,
//     does not modify the types on either the left or right
bool inferer_attempt_unify(Inferer* inferer, Type** left_ref, Type** right_ref)
{
    assert(inferer    != NULL);
    assert(left_ref   != NULL);
    assert(right_ref  != NULL);
    assert(*left_ref  != NULL);
    assert(*right_ref != NULL);

    bool is_successful = true;
    Type* left  = *left_ref ;
    Type* right = *right_ref;

    if (right->kind == TYPE_FREE_VAR)
    {
        return inferer_bind_variable_to_type(inferer, left, right);
    }

    // TODO: Rewrite this switch-case to account for attempt_unifying numerics properly.
    switch (left->kind)
    {
        case TYPE_NUMERIC    :
        case TYPE_NAT        :
        case TYPE_INT        :
        case TYPE_REAL       :
            is_successful = inferer_attempt_unify_left_type_numeric(inferer, left_ref, right_ref); break;

        case TYPE_NIL        : is_successful = right->kind == TYPE_NIL        ; break;
        case TYPE_BOOL       : is_successful = right->kind == TYPE_BOOL       ; break;
        case TYPE_STRING     : is_successful = right->kind == TYPE_STRING     ; break;
        case TYPE_LIST       : is_successful = inferer_attempt_unify_left_type_list   (inferer, left->type.list   , right_ref); break;
        case TYPE_STRUCT     : is_successful = inferer_attempt_unify_left_type_struct (inferer, left->type.structt, right_ref); break;
        case TYPE_FN         : is_successful = inferer_attempt_unify_left_type_fn     (inferer, left->type.fn     , right_ref); break;

        case TYPE_FREE_VAR   : is_successful = inferer_bind_variable_to_type(inferer, left, right); break;
        case TYPE_BOUNDED_VAR: assert(false); // Should be instantiated before this point.

        case TYPE_SCHEME     : assert(false); // Should be instantiated before this point.
    }

    return is_successful;
}

bool inferer_unify(Inferer* inferer, Type** left_ref, Type** right_ref)
{
    assert(inferer    != NULL);
    assert(left_ref   != NULL);
    assert(right_ref  != NULL);
    assert(*left_ref  != NULL);
    assert(*right_ref != NULL);

    if (!inferer_attempt_unify(inferer, left_ref, right_ref))
    {
        inferer_unify_free_binds(inferer);

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

    inferer_unify_apply_binds(inferer);
    return true;
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

    bool is_successful = false;

    is_successful = inferer_unify(inferer, type, &builtin_type_nat);
    if (is_successful)
    {
        *constraint = TYPE_CONSTRAINT_NAT;
        *type       = builtin_type_nat   ;
        return true;
    }

    is_successful = inferer_unify(inferer, type, &builtin_type_int);
    if (is_successful)
    {
        *constraint = TYPE_CONSTRAINT_INT;
        *type       = builtin_type_int   ;
        return true;
    }

    is_successful = inferer_unify(inferer, type, &builtin_type_real);
    if (is_successful)
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

    bool is_successful = false;

    is_successful = inferer_constrain_numeric(inferer, constraint, type);
    if (is_successful)
    {
        return true;
    }

    is_successful = inferer_unify(inferer, type, &builtin_type_real);
    if (is_successful)
    {
        *constraint = TYPE_CONSTRAINT_REAL;
        *type       = builtin_type_real   ;
        return true;
    }

    is_successful = inferer_unify(inferer, type, &builtin_type_nil);
    if (is_successful)
    {
        *constraint = TYPE_CONSTRAINT_NIL ;
        *type       = builtin_type_nil    ;
        return true;
    }

    is_successful = inferer_unify(inferer, type, &builtin_type_bool);
    if (is_successful)
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


    bool is_successful = true;


    inferer_infer_expr(inferer, expr, type);
    switch (*constraint)
    {
        // These are special cases:
        // Instead of unifying with a specific type, we go down the tree, and attempt to figure out which
        // of a list of types is the correct one.
        case TYPE_CONSTRAINT_NUMERIC    : is_successful = inferer_constrain_numeric (inferer, constraint, type); break;
        case TYPE_CONSTRAINT_EQUALITY   : is_successful = inferer_constrain_equality(inferer, constraint, type); break;

        case TYPE_CONSTRAINT_NIL        : is_successful = inferer_unify(inferer, type, &builtin_type_nil   ); break;
        case TYPE_CONSTRAINT_BOOL       : is_successful = inferer_unify(inferer, type, &builtin_type_bool  ); break;
        case TYPE_CONSTRAINT_NAT        : is_successful = inferer_unify(inferer, type, &builtin_type_nat   ); break;
        case TYPE_CONSTRAINT_INT        : is_successful = inferer_unify(inferer, type, &builtin_type_int   ); break;
        case TYPE_CONSTRAINT_REAL       : is_successful = inferer_unify(inferer, type, &builtin_type_real  ); break;
        case TYPE_CONSTRAINT_STRING     : is_successful = inferer_unify(inferer, type, &builtin_type_string); break;
        // default:
        //     fprintf(stderr, "[%s:%d] Type inference: Logical error while infering types.\n", __FILE__, __LINE__);
        //     exit(1);
    }

    if (!is_successful)
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

    return is_successful;
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


    bool is_successful = false;
    TypeConstraint unary_constraint = TYPE_CONSTRAINT_BOOL;


    is_successful = inferer_infer_expr_and_constrain
        (inferer, unary.unary, &unary_constraint, type);
    if (!is_successful)
        return false;

    return true;
}

bool inferer_infer_expr_unary_arithmetic_operator(Inferer* inferer, ExprUnary unary, Type** type)
{
    assert(inferer     != NULL);
    assert(unary.unary != NULL);
    assert(type        != NULL);
    assert(*type       == NULL);


    bool is_successful = false;
    TypeConstraint unary_constraint = TYPE_CONSTRAINT_NUMERIC;


    is_successful = inferer_infer_expr_and_constrain
        (inferer, unary.unary, &unary_constraint, type);
    if (!is_successful)
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


    bool is_successful = false;
    Type* left_return_type  = NULL;
    Type* right_return_type = NULL;
    TypeConstraint left_constraint  = TYPE_CONSTRAINT_NUMERIC;
    TypeConstraint right_constraint = TYPE_CONSTRAINT_NUMERIC;


    is_successful = inferer_infer_expr_and_constrain(inferer, binary.left , &left_constraint , &left_return_type );
    if (!is_successful)
        return false;

    is_successful = inferer_infer_expr_and_constrain(inferer, binary.right, &right_constraint, &right_return_type);
    if (!is_successful)
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


    bool is_successful = false;
    Type* left_return_type  = NULL;
    Type* right_return_type = NULL;
    TypeConstraint left_constraint  = TYPE_CONSTRAINT_BOOL;
    TypeConstraint right_constraint = TYPE_CONSTRAINT_BOOL;

    is_successful = inferer_infer_expr_and_constrain(inferer, binary.left , &left_constraint , &left_return_type );
    if (!is_successful)
        return false;

    is_successful = inferer_infer_expr_and_constrain(inferer, binary.right, &right_constraint, &right_return_type);
    if (!is_successful)
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


    bool is_successful = false;
    Type* left_return_type  = NULL;
    Type* right_return_type = NULL;
    TypeConstraint left_constraint  = TYPE_CONSTRAINT_EQUALITY;
    TypeConstraint right_constraint = TYPE_CONSTRAINT_EQUALITY;

    assert(false);
    is_successful = inferer_infer_expr_and_constrain(inferer, binary.left , &left_constraint , &left_return_type );
    if (!is_successful)
        return false;

    is_successful = inferer_infer_expr_and_constrain(inferer, binary.right, &right_constraint, &right_return_type );
    if (!is_successful)
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

    bool is_successful   = false;
    Type* caller_type   = NULL ;
    Type* fn_type       = NULL ;
    Type* curr_fn_ptr   = NULL ;
    Type* curr_arg_type = NULL ;

    assert(false);

    is_successful = inferer_infer_expr(inferer, fn.caller, &caller_type);
    if (!is_successful)
    {
        return false;
    }

    fn_type = inferer_create_free_function_type(inferer, fn.argc);
    curr_fn_ptr = fn_type;

    for (int i = 0; i < fn.argc; ++i)
    {
        is_successful = inferer_infer_expr(inferer, fn.argv[i], &curr_arg_type);
        if (!is_successful)
        {
            return false;
        }

        is_successful = inferer_unify(inferer, &curr_fn_ptr->type.fn.left, &curr_arg_type);
        if (!is_successful)
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
        bool is_successful = inferer_infer_stmt(inferer, inner_stmt);
        if (!is_successful)
        {
            return false;
        }
    }

    return true;
}

bool inferer_infer_stmt_while(Inferer* inferer, StmtWhile whilee)
{
    assert(inferer    != NULL);

    bool is_successful = false;
    Type* type = NULL;

    is_successful = inferer_infer_expr(inferer, whilee.condition, &type);
    if (!is_successful)
    {
        return false;
    }

    is_successful = inferer_infer_stmt(inferer, whilee.body);
    if (!is_successful)
    {
        return false;
    }

    return true;
}

bool inferer_infer_stmt_if(Inferer* inferer, StmtIf iff)
{
    assert(inferer    != NULL);

    bool is_successful = false;
    Type* type = NULL;

    is_successful = inferer_infer_expr(inferer, iff.condition, &type);
    if (!is_successful)
    {
        return false;
    }

    is_successful = inferer_infer_stmt(inferer, iff.body);
    if (!is_successful)
    {
        return false;
    }

    return true;
}

bool inferer_infer_stmt_return(Inferer* inferer, StmtReturn returnn)
{
    assert(inferer    != NULL);

    bool is_successful = false;
    Type* expr_type    = NULL ;
    Type* return_type  = NULL ;

    if (returnn.expr != NULL && !inferer_infer_expr(inferer, returnn.expr, &expr_type))
    {
        return false;
    }

    return_type = inferer_get_decl_var_return_type(inferer, returnn.fn);

    is_successful = inferer_unify(inferer, &return_type, &expr_type);
    if (!is_successful)
    {
        return false;
    }

    inferer_set_decl_var_return_type(inferer, returnn.fn, return_type);

    return true;
}

bool inferer_infer_stmt_let(Inferer* inferer, StmtLet let)
{
    assert(inferer != NULL);

    bool is_successful = false;

    Type* expr_type       = NULL;
    Type* annotation_type = NULL;
    Type* final_type      = NULL;
    Type* scheme          = NULL;

    is_successful = inferer_infer_expr(inferer, let.expr, &expr_type);
    if (!is_successful)
    {
        return false;
    }

    if (let.type != NULL)
    {
        is_successful = inferer_infer_type_expr(inferer, let.type, &annotation_type);
        if (!is_successful)
        {
            return false;
        }

        is_successful = inferer_unify(inferer, &expr_type, &annotation_type);
        if (!is_successful)
        {
            return false;
        }

        final_type = annotation_type;
    }
    else
    {
        final_type = expr_type;
    }

    is_successful = inferer_generalize(inferer, final_type, &scheme);
    if (!is_successful)
    {
        return false;
    }

    assert(scheme != NULL);
    assert(scheme->kind == TYPE_SCHEME);

    inferer_set_decl_var_type(inferer, let.decl, scheme);

    return true;
}

Type* inferer_get_stmt_fn_type(Inferer* inferer, StmtFn fn)
{
    assert(inferer != NULL);

    Type type_mem;
    Type* top_type    = NULL;
    Type* curr_branch = NULL;
    Type* return_type = NULL;

    assert(false);

    inferer_infer_type_expr(inferer, fn.return_type, &return_type);
    type_mem = (Type)
    {
        .kind = TYPE_FN,
        .type.fn = (TypeFn)
        {
            .left  = NULL       ,
            .right = return_type, 
        }
    };
    top_type = (Type*) arena_push(&inferer->type_arena, &type_mem, sizeof(Type));
    curr_branch  = top_type;

    for (int i = 0; i < fn.argc; ++i)
    {
        StmtFnArg* arg            = fn.argv[i] ;
        Type     * arg_type       = NULL       ;
        TypeExpr * arg_type_expr  = arg->type  ;
        Decl     * arg_decl       = arg->decl  ;
        Type     * old_branch     = curr_branch;

        if (arg_type_expr != NULL)
        {
            inferer_infer_type_expr(inferer, arg_type_expr, &arg_type);
        }
        else
        {
            arg_type = inferer_create_free_type_var(inferer);
        }
        inferer_set_decl_var_type(inferer, arg_decl, arg_type);

        type_mem = (Type)
        {
            .kind = TYPE_FN,
            .type.fn = (TypeFn)
            {
                .left  = arg_type                  ,
                .right = curr_branch->type.fn.right, 
            }
        };
        curr_branch = (Type*) arena_push(&inferer->type_arena, &type_mem, sizeof(Type));
        old_branch->type.fn.right = curr_branch;
    }

    return top_type;
}

bool inferer_infer_stmt_fn(Inferer* inferer, StmtFn fn)
{
    assert(inferer != NULL);

    bool is_successful = false;
    Type* fn_type     = NULL;

    assert(false);

    // What we need to do to infer the type of a function
    // We need it go inside, and resolve the individual arguments.
    // We also need to resolve all intances of return type.

    fn_type = inferer_get_stmt_fn_type(inferer, fn);
    inferer_set_decl_var_type(inferer, fn.decl, fn_type);

    is_successful = inferer_infer_stmt(inferer, fn.body);
    if (!is_successful)
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

    return true;
}

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

Type* inferer_get_decl_var_return_type(Inferer* inferer, Decl* decl)
{
    assert(inferer != NULL);
    assert(decl    != NULL);
    assert(decl->kind == DECL_VAR);
    assert(decl->decl.var.type->kind == TYPE_FN);

    Type* old_type = decl->decl.var.type;

    while (old_type->type.fn.right->kind == TYPE_FN)
    {
        old_type = old_type->type.fn.right;
    }

    return old_type->type.fn.right;
}

void inferer_set_decl_var_return_type(Inferer* inferer, Decl* decl, Type* type)
{
    assert(inferer != NULL);
    assert(decl    != NULL);
    assert(decl->kind == DECL_VAR);
    assert(decl->decl.var.type->kind == TYPE_FN);

    Type* old_type = decl->decl.var.type;

    while (old_type->type.fn.right->kind == TYPE_FN)
    {
        old_type = old_type->type.fn.right;
    }

    old_type->type.fn.right = type;
}

Type* inferer_resolve_type_variable(Inferer* inferer, Type* var)
{
    assert(inferer   != NULL         );
    assert(var       != NULL         );
    assert(var->kind == TYPE_FREE_VAR);

    Type* next = var->type.free_var.type;

    if (next == NULL || next->kind != TYPE_FREE_VAR)
    {
        return var;
    }

    return inferer_resolve_type_variable(inferer, next);
}

bool inferer_occurs_check_struct(Inferer* inferer, Type* var, TypeStruct structt)
{
    assert(inferer   != NULL         );
    assert(var       != NULL         );
    assert(var->kind == TYPE_FREE_VAR);

    Type* resolved_var = NULL;

    resolved_var = inferer_resolve_type_variable(inferer, var);
    for (int i = 0; i < structt.field_num; ++i)
    {
        TypeStructField* field = structt.fields[i];

        if (!inferer_occurs_check(inferer, resolved_var, field->value))
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

    Type* resolved_var       = NULL;
    Type* other_resolved_var = NULL;

    resolved_var = inferer_resolve_type_variable(inferer, var);
    switch (type->kind)
    {
        case TYPE_NIL        : return true;
        case TYPE_BOOL       : return true;
        case TYPE_NUMERIC    : return true;
        case TYPE_NAT        : return true;
        case TYPE_INT        : return true;
        case TYPE_REAL       : return true;
        case TYPE_STRING     : return true;
        case TYPE_LIST       : return inferer_occurs_check(inferer, resolved_var, type->type.list.type);
        case TYPE_STRUCT     : return inferer_occurs_check_struct(inferer, resolved_var, type->type.structt);
        case TYPE_FN         : return inferer_occurs_check(inferer, resolved_var, type->type.fn.left )
                                   && inferer_occurs_check(inferer, resolved_var, type->type.fn.right);
        case TYPE_FREE_VAR   :
            other_resolved_var = inferer_resolve_type_variable(inferer, type);
            return resolved_var != other_resolved_var;

        case TYPE_BOUNDED_VAR: assert(false);
        case TYPE_SCHEME     : assert(false);
    }
    fprintf(stderr, "[%s:%d] Type inference: Logical error during occurs check.\n", __FILE__, __LINE__);
    exit(1);
}

bool inferer_bind_variable_to_type(Inferer* inferer, Type* var, Type* type)
{
    assert(inferer   != NULL         );
    assert(var       != NULL         );
    assert(type      != NULL         );
    assert(var->kind == TYPE_FREE_VAR);

    bool  is_successful = false;
    Type* resolved_var  = NULL ;
    Bind  bind;

    resolved_var = inferer_resolve_type_variable(inferer, var);

    is_successful = inferer_occurs_check(inferer, resolved_var, type);
    if (!is_successful)
    {
        inferer_throw_compiler_error(inferer, (CompileError)
        {
            .kind   = ERROR_ERROR,
            .line   = -1         ,
            .column = -1         ,
            .length = -1         ,
            .msg    = "Type inference: Occurs check failed.",
        });
        return false;
    }

    bind = (Bind)
    {
        .var  = resolved_var,
        .type = type        ,
    };
    arrput(inferer->binds, bind);

    return true;
}

void inferer_unify_apply_binds(Inferer* inferer)
{
    assert(inferer != NULL);

    int length = arrlen(inferer->binds);

    assert(false);
    for (int i = 0; i < length; ++i)
    {
        Bind bind = inferer->binds[i];

        bind.var->type.free_var.type = bind.type;
    }
}

void inferer_unify_free_binds(Inferer* inferer)
{
    assert(inferer != NULL);

    arrfree(inferer->binds);
    inferer->binds = NULL  ;
}

void inferer_throw_compiler_error(Inferer* inferer, CompileError err)
{
    assert(inferer != NULL);

    CompileError* err_ptr = NULL;
    err_ptr = (CompileError*) arena_push(&inferer->arena, &err, sizeof(CompileError));
    arrput(inferer->errs, err_ptr);
}
