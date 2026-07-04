#ifndef TYPE_H
#define TYPE_H

#include "parser.h"

// 'type expression' - the type as written in a function signature, and contains positional, and other relevant information,
// 'type proper' - contains the actual type information.
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
    char* caller;
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

typedef struct Type            Type           ;
typedef enum   TypeKind        TypeKind       ;

typedef struct TypeVariable    TypeVariable   ;
typedef struct TypeList        TypeList       ;
typedef struct TypeStruct      TypeStruct     ;
typedef struct TypeStructField TypeStructField;
typedef struct TypeFn          TypeFn         ;
typedef struct TypeAlias       TypeAlias      ;
typedef struct TypeAbstraction TypeAbstraction;
typedef struct TypeApplication TypeApplication;

enum TypeKind
{
    // Primitive types
    TYPE_NIL        ,
    TYPE_BOOL       ,
    TYPE_INT        ,
    TYPE_REAL       ,
    TYPE_STRING     ,

    // Derivative types
    TYPE_LIST       ,
    TYPE_STRUCT     ,
    TYPE_FN         ,

    // Special types
    TYPE_VARIABLE   , // a type representing a yet unknown type.
    TYPE_ALIAS      , // a type representing an alias to an existing type.
    TYPE_ABSTRACTION, // a type representing a newly defined type.
    TYPE_APPLICATION, // application of abstraction
};

struct TypeVariable
{
    int id;
};

struct TypeList
{
    Type* type;
};

struct TypeStructField
{
    char* key  ;
    Type* value;
};

struct TypeStruct
{
    int field_num;
    TypeStructField** fields;
};

struct TypeFn
{
    Type* left ;
    Type* right;
};

struct TypeAlias
{
    Type* type;
};

struct TypeAbstraction
{
    int    parameter_num  ;
    int    constructor_num;
    Type** parameters     ; // All of kind TYPE_VARIABLE
    Type** constructors   ; // All of kind TYPE_FN, where the rightmost node_ptr is equal of the 'TypeNewType' itself.
};

struct TypeApplication
{
    Type*  abstraction;
    int    argc;
    Type** argv;
};

// Not implemented yet
struct Type
{
    TypeKind kind;
    union
    {
        void          * none       ; // Primitives and variable. (should be set to NULL in that case).
        TypeVariable    variable   ;
        TypeList        list       ;
        TypeStruct      structt    ;
        TypeFn          fn         ;
        TypeAlias       alias      ;
        TypeAbstraction abstraction;
        TypeApplication application;
    }
    type;
};

// Type* type_convert_type_expr_to_type(Arena* arena, TypeExpr* type_expr);

#endif
