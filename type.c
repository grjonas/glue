#include "type.h"

static Type private_builtin_type_nil     = (Type) { .kind = TYPE_NIL     };
static Type private_builtin_type_bool    = (Type) { .kind = TYPE_BOOL    };
static Type private_builtin_type_nat     = (Type) { .kind = TYPE_NAT     };
static Type private_builtin_type_int     = (Type) { .kind = TYPE_INT     };
static Type private_builtin_type_real    = (Type) { .kind = TYPE_REAL    };
static Type private_builtin_type_string  = (Type) { .kind = TYPE_STRING  };

Type* builtin_type_nil     = &private_builtin_type_nil    ;
Type* builtin_type_bool    = &private_builtin_type_bool   ;
Type* builtin_type_nat     = &private_builtin_type_nat    ;
Type* builtin_type_int     = &private_builtin_type_int    ;
Type* builtin_type_real    = &private_builtin_type_real   ;
Type* builtin_type_string  = &private_builtin_type_string ;

// Returns NULL on failure.
// NOTE: Binary search, as far as I remember, requires the array to be sorted,
// which is not the case here. So linear search is the answer.
TypeStructField* type_struct_find_key(TypeStruct structt, char* key)
{
    for (int i = 0; i < structt.field_num; ++i)
    {
        TypeStructField* field = structt.fields[i];
        if (strcmp(field->key, key) == 0)
        {
            return field;
        }
    }

    return NULL;
}

bool type_kind_is_numeric(TypeKind kind)
{
    switch (kind)
    {
        case TYPE_NAT : return true;
        case TYPE_INT : return true;
        case TYPE_REAL: return true;
        default:
            return false;
    }
}

bool type_kind_is_equality(TypeKind kind)
{
    switch (kind)
    {
        case TYPE_NIL   : return true;
        case TYPE_BOOL  : return true;
        case TYPE_NAT   : return true;
        case TYPE_INT   : return true;
        case TYPE_REAL  : return true;
        case TYPE_STRING: return true;
        default:
            return false;
    }
}

// TODO: Not efficient in an imperative language, rewrite this later.
int get_type_fn_arg_num(Type* type)
{
    assert(type != NULL);
    assert(type->kind == TYPE_FN);
    assert(type->type.fn.right != NULL);

    Type* left  = type->type.fn.left ;
    Type* right = type->type.fn.right;

    if (right->kind == TYPE_FN)
    {
        assert(left != NULL);

        return 1 + get_type_fn_arg_num(right);
    }
    else
    {
        return left == NULL ? 0 : 1;
    }
}

Type* get_type_fn_return_type(Type* type)
{
    assert(type != NULL);
    assert(type->kind == TYPE_FN);
    assert(type->type.fn.right != NULL);

    Type* right = type->type.fn.right;

    if (right->kind == TYPE_FN)
    {
        return get_type_fn_return_type(right);
    }
    else
    {
        return right;
    }
}

// TODO: Not efficient in an imperative language, rewrite this later.
int get_type_constructor_arg_num(Type* type)
{
    assert(type != NULL);
    assert(type->kind == TYPE_CONSTRUCTOR);
    assert(type->type.fn.right != NULL);

    Type* left  = type->type.fn.left ;
    Type* right = type->type.fn.right;

    if (right->kind == TYPE_CONSTRUCTOR)
    {
        assert(left != NULL);

        return 1 + get_type_constructor_arg_num(right);
    }
    else
    {
        return left == NULL ? 0 : 1;
    }
}

Type* get_type_constructor_return_type(Type* type)
{
    assert(type != NULL);
    assert(type->kind == TYPE_CONSTRUCTOR);
    assert(type->type.fn.right != NULL);

    Type* right = type->type.fn.right;

    if (right->kind == TYPE_CONSTRUCTOR)
    {
        return get_type_constructor_return_type(right);
    }
    else
    {
        return right;
    }
}
