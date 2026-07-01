#ifndef TYPE_H
#define TYPE_H

#include "parser.h"

typedef struct Type Type;

// Not implemented yet
struct Type
{
    void* none;
};

// TODO: We need to establish a difference between a 'type expression' - the type as written in a function signature, and contains positional, and other relevant information,
// and the 'type proper', which contains the actual type information.
typedef struct TypeExpr            TypeExpr           ;
typedef enum   TypeExprKind        TypeExprKind       ;

typedef struct TypeExprIdentifier  TypeExprIdentifier ;
typedef struct TypeExprNil         TypeExprNil        ;
typedef struct TypeExprBool        TypeExprBool       ;
typedef struct TypeExprInt         TypeExprInt        ;
typedef struct TypeExprReal        TypeExprReal       ;
typedef struct TypeExprString      TypeExprString     ;
typedef struct TypeExprList        TypeExprList       ;
typedef struct TypeExprStructField TypeExprStructField;
typedef struct TypeExprStruct      TypeExprStruct     ;
typedef struct TypeExprFunction    TypeExprFunction   ; // example: map : (a -> b) -> [a] -> [b]
typedef struct TypeExprInstance    TypeExprInstance   ; // example: Maybe(a), which can be instantiated as such - Maybe(Int), etc.

enum TypeExprKind
{
    TYPE_EXPR_IDENTIFIER,
    TYPE_EXPR_NIL       ,
    TYPE_EXPR_BOOL      ,
    TYPE_EXPR_INT       ,
    TYPE_EXPR_REAL      ,
    TYPE_EXPR_STRING    ,
    TYPE_EXPR_LIST      ,
    TYPE_EXPR_STRUCT    ,
    TYPE_EXPR_FN        ,
    TYPE_EXPR_INSTANCE  ,
};

struct TypeExprIdentifier
{
    char* identifier;
};

struct TypeExprList
{
    TypeExpr* type;
};

struct TypeExprStructField
{
    char* key;
    TypeExpr* value;
};

struct TypeExprStruct
{
    int argc;
    TypeExprStructField** argv;
};

struct TypeExprFunction
{
    TypeExpr* left ;
    TypeExpr* right;
};

struct TypeExprInstance
{
    TypeExpr* caller;
    int argc;
    TypeExpr** argv;
};

// Holds type information.
// Each 'TypeExpr' object is considered a seperate 'variable', which is why if the kind of type is TYPE_VARIABLE, there is not type id of any kind.
struct TypeExpr
{
    TypeExprKind kind;
    int line  ;
    int column;
    int length;

    union
    {
        void             * none     ; // Primitives don't need any data, so they're just NULL void pointers;
        TypeExprIdentifier identifier;
        TypeExprList       list      ;
        TypeExprStruct     structt   ;
        TypeExprFunction   fn        ;
        TypeExprInstance   instance  ;
    }
    type_expr;
};

TypeExpr* parser_parse_type_expr          (Parser* parser);

TypeExpr* parser_parse_type_expr_primitive(Parser* parser);
TypeExpr* parser_parse_type_expr_list     (Parser* parser);
TypeExpr* parser_parse_type_expr_struct   (Parser* parser);
TypeExpr* parser_parse_type_expr_function (Parser* parser);
TypeExpr* parser_parse_type_expr_instance (Parser* parser);

#endif
