#ifndef DECL_H
#define DECL_H

#include "dependencies.h"
#include "type.h"

typedef struct Decl                Decl               ;
typedef enum   DeclKind            DeclKind           ;

typedef struct DeclVar             DeclVar            ;
typedef struct DeclTypeVariable    DeclTypeVariable   ;
typedef struct DeclAlias           DeclAlias          ;
typedef struct DeclTypeConstructor DeclTypeConstructor;
typedef struct DeclType            DeclType           ;

enum DeclKind
{
    DECL_LET             ,
    DECL_TYPE_VARIABLE   ,
    DECL_ALIAS           ,
    DECL_TYPE            ,
    DECL_TYPE_CONSTRUCTOR,
};

struct Decl
{
    DeclKind kind;
    char* identifier;
    Type* type      ;
    int   id        ;
};

bool decl_is_type_variable(Decl decl);
bool decl_is_alias(Decl decl);
bool decl_is_new_type(Decl decl);
int  decl_get_new_type_parameter_num(Decl decl);

#endif
