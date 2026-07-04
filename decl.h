#ifndef DECL_H
#define DECL_H

#include "expr.h"
#include "type.h"

typedef struct Resolver Resolver;

typedef struct Decl                Decl               ;
typedef enum   DeclKind            DeclKind           ;

typedef struct DeclLet             DeclLet            ;
// typedef struct DeclFn              DeclFn             ;
typedef struct DeclTypeVariable    DeclTypeVariable   ;
typedef struct DeclAlias           DeclAlias          ;
typedef struct DeclTypeConstructor DeclTypeConstructor;
typedef struct DeclType            DeclType           ;

enum DeclKind
{
    DECL_LET             ,
    // DECL_FN              ,
    DECL_TYPE_VARIABLE   ,
    DECL_ALIAS           ,
    DECL_TYPE            ,
    DECL_TYPE_CONSTRUCTOR,
};

// struct DeclLet
// {
// };
// 
// // struct DeclFn
// // {
// //     Variable  var;
// //     int argc;
// //     Variable** argv;
// // };
// 
// struct DeclTypeVariable
// {
// };
// 
// struct DeclAlias
// {
// };
// 
// struct DeclType
// {
// };
// 
// struct DeclTypeConstructor // Has to be declared after a 'DeclType'
// {
// };
// 
// Declaration
struct Decl
{
    DeclKind kind;
    char* identifier;
    Type* type      ;
    int   id        ;
//     struct
//     {
//         DeclLet             let        ;
//         // DeclFn              fn         ;
//         DeclTypeVariable    type_var   ;
//         DeclAlias           alias      ;
//         DeclType            type       ;
//         DeclTypeConstructor constructor;
//     }
//     decl;
};

#endif
