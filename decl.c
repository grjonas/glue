#include "decl.h"

extern bool decl_is_variable(Decl decl)
{
    return decl.kind == DECL_VAR;
}

extern bool decl_is_type_variable(Decl decl)
{
    return decl.kind == DECL_TYPE_VAR;
}

extern bool decl_is_alias(Decl decl)
{
    return decl.kind == DECL_ALIAS;
}

extern bool decl_is_new_type(Decl decl)
{
    return decl.kind == DECL_TYPE;
}

extern int decl_get_new_type_parameter_num(Decl decl)
{
    assert(decl.kind == DECL_TYPE);

    return decl.decl.type.type_var_num;
}

extern bool decl_is_type_constructor(Decl decl)
{
    return decl.kind == DECL_TYPE_CONSTRUCTOR;
}

extern int decl_get_type_constructor_parameter_num(Decl decl)
{
    assert(decl.kind == DECL_TYPE_CONSTRUCTOR);

    return decl.decl.constructor.type_num;
}
