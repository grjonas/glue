#ifndef TYPE_H
#define TYPE_H

#include "parser.h"

typedef struct Type             Type          ;
typedef enum   TypeKind         TypeKind      ;

typedef struct TypeList        TypeList       ;
typedef struct TypeStructField TypeStructField;
typedef struct TypeStruct      TypeStruct     ;
typedef struct TypeFunction    TypeFunction   ;

// Type
enum TypeKind
{
    // Special
    TYPE_UNKNOWN   ,
    TYPE_VARIABLE  ,
    TYPE_IDENTIFIER,
    TYPE_ALIAS     ,

    TYPE_PRIMITIVE_SEPERATOR,

    // Built-in primitives
    TYPE_NIL       ,
    TYPE_BOOL      ,
    TYPE_INT       , // i64 - for now at least
    TYPE_REAL      , // f64 - for now at least

    TYPE_DERIVATIVE_SEPERATOR,

    // Built-in derivative
    TYPE_LIST      ,
    TYPE_STRUCT    ,
    TYPE_FN        ,

    TYPE_MISC_SEPERATOR,
};

// Tagged Union

struct TypeList
{
    Type* type;
};

struct TypeFunction
{
    int   argc;
    Type** argv;
};

struct TypeStructField
{
    char* key;
    Type* value;
};

struct TypeStruct
{
    int argc;
    TypeStructField** argv;
};

struct Type
{
    TypeKind kind;
    char* identifier;
    int line  ;
    int column;
    int length;

    union
    {
        void        * primitive; // Primitives don't have any elements, so this would be NULL.
        TypeList      list     ;
        TypeStruct    structt  ;
        TypeFunction  fn       ;
    }
    type;
};

// Type parsing
Type* parser_parse_type(Parser* parser);

Type* parser_parse_type_primitive(Parser* parser);
Type* parser_parse_type_list(Parser* parser);
Type* parser_parse_type_struct(Parser* parser);
Type* parser_parse_type_function(Parser* parser);

#endif
