#ifndef TYPE_H
#define TYPE_H

#include "parser.h"

typedef struct Type             Type          ;
typedef enum   TypeKind         TypeKind      ;

typedef struct TypeList        TypeList       ;
typedef struct TypeStructField TypeStructField;
typedef struct TypeStruct      TypeStruct     ;
typedef struct TypeGeneric     TypeGeneric    ;
typedef struct TypeFunction    TypeFunction   ;

// Type
enum TypeKind
{
    // Special
    TYPE_ERROR     ,
    TYPE_UNKNOWN   ,
    TYPE_VARIABLE  ,
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

    TYPE_GENERIC   ,
};

struct TypeList
{
    Type* type;
};

struct TypeGeneric
{
    char* identifier;
    int   argc;
    Type** argv;
};

struct TypeFunction
{
    int argc;
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

// Holds type information.
// Each 'Type' object is considered a seperate 'variable', which is why if the kind of type is TYPE_VARIABLE, there is not type id of any kind.
struct Type
{
    TypeKind kind;
    int line  ;
    int column;
    int length;

    union
    {
        void        * none     ; // Primitives and variables don't need any data, so they're just NULL void pointers;
        TypeList      list     ;
        TypeStruct    structt  ;
        TypeGeneric   generic  ;
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
