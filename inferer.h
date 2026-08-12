#ifndef INFERER_H
#define INFERER_H

#include "resolver.h"

// TODO: Don't forget to refactor inferer init and free after changing resolver.
typedef struct Inferer          Inferer         ;
typedef enum   TypeConstraint   TypeConstraint  ;
typedef struct Bind             Bind            ;
typedef struct TypeEnv          TypeEnv         ;
typedef struct Subst            Subst           ;

enum TypeConstraint
{
    TYPE_CONSTRAINT_NIL        ,
    TYPE_CONSTRAINT_BOOL       ,
    TYPE_CONSTRAINT_NUMERIC    ,
    TYPE_CONSTRAINT_NAT        ,
    TYPE_CONSTRAINT_INT        ,
    TYPE_CONSTRAINT_REAL       ,
    TYPE_CONSTRAINT_STRING     ,
    TYPE_CONSTRAINT_EQUALITY   ,
    TYPE_CONSTRAINT_LIST       ,
    TYPE_CONSTRAINT_VAR        ,
};

struct Bind
{
    Type** var_ref;
    Type*     type;
};

struct TypeEnv
{
    int type_variable_length;
};

struct Subst
{
    Type*   key;
    Type* value;
};

// There are a couple of things that should be known about the inferer:
// 1) Unlike some other components, we benefit from not using a arenas as religiously as we did before.
//     Instead, we should have a 'Subst**', which would allow us to deallocate, and reallocate memory at will.
struct Inferer
{
    // Inputs
    const char* filename;
    const char* txt;
    DYNAMIC_ARRAY(Token* tokens);
    Stmt*  stmts;
    DYNAMIC_ARRAY(Decl** declarations); // Holds ALL scanned declarations
    DYNAMIC_ARRAY(char** identifiers );

    // Memory-management
    Arena arena;
    Arena type_arena;

    // Misc. state
    DYNAMIC_ARRAY(Type**) type_variables; // A stack of type variables
    DYNAMIC_ARRAY(Bind *) binds;
    TypeEnv curr_type_env;

    // Outputs

    // Errs
    DiagnosticComponent* diagnostic_component;
};

extern Inferer inferer_init(Resolver* resolver);
extern void    inferer_free(Inferer* inferer  );

extern bool  inferer_resolve             (Inferer* inferer, Type* type, Type** resolved_type)   ; // Takes a type, and attempts to find the bottom-most concrete type in the type graph.
extern bool  inferer_unify_inner         (Inferer* inferer, Type** left_ref, Type** right_ref)  ; // Unifies the two types
extern bool  inferer_attempt_unify       (Inferer* inferer, Type** left_ref, Type** right_ref)  ;
extern bool  inferer_unify               (Inferer* inferer, Type** left_ref, Type** right_ref, Span span); // Unifies the two types
extern void  inferer_generalize          (Inferer* inferer, Type* type, TypeScheme** scheme)    ;
extern void  inferer_instantiate         (Inferer* inferer, TypeScheme* scheme, Type** type)    ;

extern bool  inferer_infer_expr          (Inferer* inferer, Expr* expr, Type** type)            ;
extern void  inferer_convert_type_expr   (Inferer* inferer, TypeExpr* type_expr, Type** type)   ;
extern bool  inferer_infer_pattern       (Inferer* inferer, Pattern* pattern, Type* type)       ;
extern bool  inferer_infer_stmt          (Inferer* inferer, Stmt* stmt)                         ;

#endif
