#ifndef TYPE_H
#define TYPE_H

#include "parser.h"

typedef struct Type         Type        ;
typedef enum   TypeKind     TypeKind    ;

typedef struct TypeFunction TypeFunction;
typedef struct TypeStruct   TypeStruct  ;

// Type
enum TypeKind
{
    // Special
    TYPE_UNKNOWN  ,

    TYPE_PRIMITIVE_SEPERATOR,

    // Built-in primitives
    // TYPE_TYPE     ,
    TYPE_NIL      ,
    TYPE_BOOL     ,
    TYPE_INT      , // i64 - for now at least
    TYPE_REAL     , // f64 - for now at least

    TYPE_DERIVATIVE_SEPERATOR,

    // Built-in derivative
    TYPE_LIST     ,
    TYPE_STRUCT   ,
    TYPE_FUNCTION ,

    TYPE_MISC_SEPERATOR,

    // TYPE_SEPERATOR, // Used to seperate the built-in types, from newly created ones
};

// Tagged Union
struct Type
{
    TypeKind kind;

    union
    {
        void        * primitive  ; // Primitives don't have any elements, so this would be NULL.
        TypeKind      list       ;
        TypeFunction* function   ;
        TypeStruct  * struct_type;
    }
    type;
};

struct TypeFunction
{
    int   argc;
    Type* argv;
    Type* return_type;
};

// Type parsing
Type* parser_parse_type(Parser* parser);
Type* parser_parse_type_inner(Parser* parser);

#endif // type_h_INCLUDED
