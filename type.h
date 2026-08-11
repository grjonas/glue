#ifndef TYPE_H
#define TYPE_H

#include "dependencies.h"
#include "decl_definition.h"

typedef struct Type            Type           ;
typedef enum   TypeKind        TypeKind       ;

typedef struct TypeFreeVar     TypeFreeVar    ;
typedef struct TypeBoundedVar  TypeBoundedVar ;
typedef struct TypeList        TypeList       ;
typedef struct TypeStruct      TypeStruct     ;
typedef struct TypeStructField TypeStructField;
typedef struct TypeFn          TypeFn         ;
typedef struct TypeScheme      TypeScheme     ;
typedef struct TypeAbstraction TypeAbstraction;
typedef struct TypeApplication TypeApplication;
typedef struct TypeConstructor TypeConstructor;
typedef struct TypeAlias       TypeAlias      ;

// typedef struct TypeAlias       TypeAlias      ;

enum TypeKind
{
    // Primitive types
    TYPE_NIL        ,
    TYPE_BOOL       ,
    TYPE_NAT        ,
    TYPE_INT        ,
    TYPE_REAL       ,
    TYPE_STRING     ,

    // Derivative types
    TYPE_LIST       ,
    TYPE_STRUCT     ,
    TYPE_FN         ,

    // Special types
    TYPE_FREE_VAR   ,
    TYPE_BOUNDED_VAR,

    TYPE_APPLICATION, // instance of abstraction
    TYPE_CONSTRUCTOR, // basically a function,

    TYPE_ALIAS      , // a type representing an alias to an existing type.
    // TYPE_SCHEME     ,
};

struct TypeFreeVar
{
};

struct TypeBoundedVar
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

struct TypeAbstraction
{
    int argc;
};

struct TypeApplication
{
    Decl*  decl; // Points to a type declaration
    Type** argv;
    int    argc; // should be the same number of arguments as DeclType in decl.
};

struct TypeConstructor
{
    Type* left ;
    Type* right;
};

struct TypeAlias
{
    Type* type;
};

struct TypeScheme
{
    int quantified_count;
    Type* type;
};

struct Type
{
    TypeKind kind;
    union
    {
        TypeFreeVar     free_var   ;
        TypeBoundedVar  bounded_var;
        TypeList        list       ;
        TypeStruct      structt    ;
        TypeFn          fn         ;
        TypeApplication application;
        TypeConstructor constructor;
        TypeAlias       alias      ;
    }
    type;
};

TypeStructField* type_struct_find_key(TypeStruct structt, char* key);
bool type_kind_is_numeric (TypeKind kind);
bool type_kind_is_equality(TypeKind kind);

#endif
