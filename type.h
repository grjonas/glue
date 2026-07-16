#ifndef TYPE_H
#define TYPE_H

typedef struct Type            Type           ;
typedef enum   TypeKind        TypeKind       ;

typedef struct TypeFreeVar     TypeFreeVar    ;
typedef struct TypeBoundedVar  TypeBoundedVar ;
typedef struct TypeList        TypeList       ;
typedef struct TypeStruct      TypeStruct     ;
typedef struct TypeStructField TypeStructField;
typedef struct TypeFn          TypeFn         ;
// typedef struct TypeAlias       TypeAlias      ;
// typedef struct TypeAbstraction TypeAbstraction;
// typedef struct TypeApplication TypeApplication;

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
    TYPE_FREE_VAR,
    TYPE_BOUNDED_VAR,
    // TYPE_ALIAS      , // a type representing an alias to an existing type.
    // TYPE_ABSTRACTION, // a type representing a newly defined type.
    // TYPE_APPLICATION, // application of abstraction
};

struct TypeFreeVar
{
    Type* type;
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

struct Type
{
    TypeKind kind;
    union
    {
        TypeFreeVar    free_var   ;
        TypeBoundedVar bounded_var;
        TypeList       list       ;
        TypeStruct     structt    ;
        TypeFn         fn         ;
    }
    type;
};

// struct TypeAbstraction
// {
//     int    parameter_num  ;
//     int    constructor_num;
//     Type** parameters     ; // All of kind TYPE_VARIABLE
//     Type** constructors   ; // All of kind TYPE_FN, where the rightmost node_ptr is equal of the 'TypeNewType' itself.
// };
// 
// struct TypeApplication
// {
//     Type*  abstraction;
//     int    argc;
//     Type** argv;
// };

#endif
