#include "type.h"

// Type
Type* parser_parse_type(Parser* parser)
{
    return parser_parse_type_inner(parser);
}

// For now can parse only primitive types.
Type* parser_parse_type_inner(Parser* parser)
{
    Type  outer_type ;
    Type* type = NULL;

    Token token;

    outer_type = (Type)
    {
        .kind           = TYPE_UNKNOWN,
        .body.primitive = NULL        , // Assumes primitive, though this should set all pointers to NULL.
    };

    token = parser_next(parser);
    switch(token.type)
    {
        case TOKEN_NIL_T:
            outer_type.kind = TYPE_NIL ;
            break;
        case TOKEN_BOOL :
            outer_type.kind = TYPE_BOOL;
            break;
        case TOKEN_INT  :
            outer_type.kind = TYPE_INT;
            break;
        case TOKEN_REAL :
            outer_type.kind = TYPE_REAL;
            break;
        default:
            fprintf(stderr, "[%s:%d] Type parsing: Unexpected token encountered.\n", __FILE__, __LINE__);
            exit(1);
    }

    type = (Type*) arena_push(&parser->arena, &outer_type, sizeof(Type));

    return type;
}

Type* construct_type_unknown(Arena* arena)
{
    Type* type = NULL;
    Type  type_mem   ;

    type_mem = (Type)
    {
        .kind           = TYPE_UNKNOWN,
        .body.primitive = NULL        ,
    };
    type = (Type*) arena_push(arena, &type_mem, sizeof(Type));

    return type;
}

Type* construct_type_primitive(Arena* arena, TypeKind kind)
{
    Type* type = NULL;
    Type  type_mem   ;

    if (kind <= TYPE_PRIMITIVE_SEPERATOR || TYPE_DERIVATIVE_SEPERATOR <= kind)
    {
        fprintf(stderr, "[%s:%d] Type construction: Cannot construct primitive type by passing a non-primitive TypeKind.\n", __FILE__, __LINE__);
        exit(1);
    }

    type_mem = (Type)
    {
        .kind           = kind,
        .body.primitive = NULL,
    };
    type = (Type*) arena_push(arena, &type_mem, sizeof(Type));

    return type;
}


Type* construct_type_function(Arena* arena, int argc, Type* argv, Type* return_type)
{
    Type        * type          = NULL;
    TypeFunction* type_function = NULL;
    Type        * type_args     = NULL;

    Type         type_mem;
    TypeFunction func_mem;  

    type_args = (Type*) arena_push(arena, argv, (size_t) argc * sizeof(Type));

    func_mem = (TypeFunction)
    {
        .argc        = argc       ,
        .argv        = type_args  ,
        .return_type = return_type,
    };
    type_function = (TypeFunction*) arena_push(arena, &func_mem, sizeof(TypeFunction));
    if (type_function == NULL)
    {
        fprintf(stderr, "[%s:%d] Type construction: Failed to push to arena.\n", __FILE__, __LINE__);
        exit(1);
    }

    type_mem = (Type)
    {
        .kind          = TYPE_FUNCTION,
        .body.function = type_function,
    };
    type = (Type*) arena_push(arena, &type_mem, sizeof(Type));
    if (type == NULL)
    {
        fprintf(stderr, "[%s:%d] Type construction: Failed to push to arena.\n", __FILE__, __LINE__);
        exit(1);
    }

    return type;
}
