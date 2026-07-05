#ifndef TYPE_H
#define TYPE_H

typedef struct Type Type;

struct Type
{
};

// typedef struct Type Type;
// 
// typedef struct Type            Type           ;
// typedef enum   TypeKind        TypeKind       ;
// 
// typedef struct TypeVariable    TypeVariable   ;
// typedef struct TypeList        TypeList       ;
// typedef struct TypeStruct      TypeStruct     ;
// typedef struct TypeStructField TypeStructField;
// typedef struct TypeFn          TypeFn         ;
// typedef struct TypeAlias       TypeAlias      ;
// typedef struct TypeAbstraction TypeAbstraction;
// typedef struct TypeApplication TypeApplication;
// 
// enum TypeKind
// {
//     // Primitive types
//     TYPE_NIL        ,
//     TYPE_BOOL       ,
//     TYPE_INT        ,
//     TYPE_REAL       ,
//     TYPE_STRING     ,
// 
//     // Derivative types
//     TYPE_LIST       ,
//     TYPE_STRUCT     ,
//     TYPE_FN         ,
// 
//     // Special types
//     TYPE_VARIABLE   , // a type representing a yet unknown type.
//     TYPE_ALIAS      , // a type representing an alias to an existing type.
//     TYPE_ABSTRACTION, // a type representing a newly defined type.
//     TYPE_APPLICATION, // application of abstraction
// };
// 
// struct TypeVariable
// {
//     int id;
// };
// 
// struct TypeList
// {
//     Type* type;
// };
// 
// struct TypeStructField
// {
//     char* key  ;
//     Type* value;
// };
// 
// struct TypeStruct
// {
//     int field_num;
//     TypeStructField** fields;
// };
// 
// struct TypeFn
// {
//     Type* left ;
//     Type* right;
// };
// 
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
// 
// struct Type
// {
//     TypeKind kind;
//     union
//     {
//         void          * none       ; // Primitives and variable. (should be set to NULL in that case).
//         TypeVariable    variable   ;
//         TypeList        list       ;
//         TypeStruct      structt    ;
//         TypeFn          fn         ;
//         TypeAbstraction abstraction;
//         TypeApplication application;
//     }
//     type;
// };
// 
// struct Scheme
// {
//     int argc;
// };

#endif
