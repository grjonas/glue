#ifndef DECL_H
#define DECL_H

#include "dependencies.h"
#include "decl_definition.h"
#include "type_expr.h"
#include "type.h"

typedef enum   DeclKind            DeclKind           ;

typedef struct DeclVar             DeclVar            ;
typedef struct DeclTypeVar         DeclTypeVar        ;
typedef struct DeclAlias           DeclAlias          ;
typedef struct DeclTypeConstructor DeclTypeConstructor;
typedef struct DeclType            DeclType           ;

enum DeclKind
{
    DECL_VAR             ,
    DECL_TYPE_VAR        ,
    DECL_ALIAS           ,
    DECL_TYPE            ,
    DECL_TYPE_CONSTRUCTOR,
};

struct DeclVar
{
    Type* type      ;
};

struct DeclTypeVar
{
};

struct DeclAlias
{
    Type* type      ;
    TypeExpr* type_expr;
};

struct DeclType
{
    Decl** type_vars;
    Decl** constructors;
    int type_var_num;
    int constructor_num;
};

struct DeclTypeConstructor
{
    TypeExpr** types;
    int type_num;
};

struct Decl
{
    DeclKind kind;
    int   id        ;
    char* identifier;
    union
    {
        DeclVar             var        ;
        DeclTypeVar         type_var   ;
        DeclAlias           alias      ;
        DeclType            type       ;
        DeclTypeConstructor constructor;
    }
    decl;
};

bool decl_is_type_variable(Decl decl);
bool decl_is_alias(Decl decl);
bool decl_is_new_type(Decl decl);
int  decl_get_new_type_parameter_num(Decl decl);

#endif
