#include "type.h"

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
