#ifndef DECL_H
#define DECL_H

#include "dependencies.h"
#include "ast_definitions.h"
#include "type_expr.h"
#include "type.h"

typedef enum   DeclKind            DeclKind           ;

typedef struct DeclVar             DeclVar            ;
typedef struct DeclTypeVar         DeclTypeVar        ;
typedef struct DeclAlias           DeclAlias          ;
typedef struct DeclType            DeclType           ;
typedef struct DeclTypeConstructor DeclTypeConstructor;

typedef struct DeclInferredType    DeclInferredType   ;
typedef struct DeclInferredTypeConstructor DeclInferredTypeConstructor;

enum DeclKind
{
    DECL_VAR             ,
    DECL_TYPE_VAR        ,
    DECL_ALIAS           ,
    DECL_TYPE            ,
    DECL_TYPE_CONSTRUCTOR,
};

typedef enum
{
    DECL_VAR_RETURN_NONE,
    DECL_VAR_RETURN_NULL,
    DECL_VAR_RETURN_NOT_NULL,
}
DeclVarReturnKind;

struct DeclVar
{
    Type*         type;
    TypeScheme* scheme;
    DeclVarReturnKind return_kind;  // This field does nothing if the declaration is not a function.
};

struct DeclTypeVar
{
    Type* type;
};

struct DeclAlias
{
    Type* type      ;
    TypeExpr* type_expr;
};

struct DeclType
{
    TypeAbstraction* abstraction;
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

struct DeclInferredType
{
};

struct DeclInferredTypeConstructor
{
    Decl* decl_type;
    Type** types;
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

        // Below is the mechanism for declaring new types,
        // and their respective type expressions:
        // * Exist after resolution step:
        DeclType            type       ;
        DeclTypeConstructor constructor;

        // * Exist after inferrence step:
        DeclInferredType    inferred_type;
        DeclInferredTypeConstructor inferred_type_constructor;

        // The reasons for this seperation is as follows:
        // after the resolution step, we have an incomplete
        // model of the declaration, since the inferrence step hasn't
        // started yet, so we simply can't model the types yet.
        // If 'DeclType' was to be reused rather than replaced,
        // the struct would have a lot of vestigial fields,
        // which aren't really useful after inference.
        // Same goes for 'DeclTypeConstructor'.
        // 'DeclAlias' has a similiar problem, but the amount of data contained in it
        // is relatively small, so it's not as important.
    }
    decl;
};

extern bool decl_is_variable(Decl decl);
extern bool decl_is_type_variable(Decl decl);
extern bool decl_is_alias(Decl decl);
extern bool decl_is_new_type(Decl decl);
extern int  decl_get_new_type_parameter_num(Decl decl);
extern bool decl_is_type_constructor(Decl decl);
extern int decl_get_type_constructor_parameter_num(Decl decl);

#endif
