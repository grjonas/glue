#include "decl.h"

bool decl_is_type_variable(Decl decl)
{
    return decl.kind == DECL_TYPE_VARIABLE;
}

bool decl_is_alias(Decl decl)
{
    return decl.kind == DECL_ALIAS;
}

bool decl_is_new_type(Decl decl)
{
    return decl.kind == DECL_TYPE;
}

int  decl_get_new_type_parameter_num(Decl decl)
{
    fprintf(stderr, "[%s:%d] Declarations: Not implemented yet.\n", __FILE__, __LINE__);
    exit(1);
}
