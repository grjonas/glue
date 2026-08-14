#ifndef TYPE_H
#define TYPE_H

#include "dependencies.h"
#include "decl_definition.h"

typedef struct Type Type;

typedef enum
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
}
TypeKind;

typedef struct
{
}
TypeFreeVar;

typedef struct
{
    int id;
}
TypeBoundedVar;

typedef struct
{
    Type* type;
}
TypeList;

typedef struct
{
    char* key  ;
    Type* value;
}
TypeStructField;

typedef struct
{
    int field_num;
    TypeStructField** fields;
}
TypeStruct;

typedef struct
{
    Type* left ;
    Type* right;
}
TypeFn;

typedef struct
{
    Decl*  decl; // Points to a type declaration
    Type** argv;
    int    argc; // should be the same number of arguments as DeclType in decl.
}
TypeApplication;

typedef struct
{
    Type* left ;
    Type* right;
}
TypeConstructor;

typedef struct
{
    Type* type;
}
TypeAlias;

typedef struct
{
    int quantified_count;
    Type* type;
}
TypeScheme;

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

extern Type* builtin_type_nil   ;
extern Type* builtin_type_bool  ;
extern Type* builtin_type_nat   ;
extern Type* builtin_type_int   ;
extern Type* builtin_type_real  ;
extern Type* builtin_type_string;

TypeStructField* type_struct_find_key(TypeStruct structt, char* key);
bool type_kind_is_numeric (TypeKind kind);
bool type_kind_is_equality(TypeKind kind);
int get_type_fn_arg_num(Type* type);
Type* get_type_fn_return_type(Type* type);
int get_type_constructor_arg_num(Type* type);
Type* get_type_constructor_return_type(Type* type);

#endif
