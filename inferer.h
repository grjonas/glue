#ifndef INFERER_H
#define INFERER_H

#include "resolver.h"
#include "type.h"

// TODO: Don't forget to refactor inferer init and free after changing resolver.
typedef struct Inferer         Inferer        ;
typedef enum   TypeConstraint  TypeConstraint ;
typedef struct Bind            Bind           ;

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
    Type* var ;
    Type* type;
};

// There are a couple of things that should be known about the inferer:
// 1) Unlike some other components, we benefit from not using a arenas as religiously as we did before.
//     Instead, we should have a 'Subst**', which would allow us to deallocate, and reallocate memory at will.
struct Inferer
{
    // Inputs
    const char* txt;
    DYNAMIC_ARRAY(Token* tokens);
    Stmt*  stmts;
    DYNAMIC_ARRAY(Decl** declarations); // Holds ALL scanned declarations
    DYNAMIC_ARRAY(char** identifiers );

    // Memory-management
    Arena arena;
    Arena type_arena;

    // Misc. state
    DYNAMIC_ARRAY(Type** type_variables);
    DYNAMIC_ARRAY(Bind * binds);

    // Outputs

    // Errs
    DYNAMIC_ARRAY(CompileError** errs);
};

Inferer inferer_init(Resolver* resolver);
void    inferer_free(Inferer* inferer  );

bool inferer_infer_stmt      (Inferer* inferer, Stmt* stmt)                       ;
bool inferer_infer_type_expr (Inferer* inferer, TypeExpr* type_expr, Type** type) ;
bool inferer_infer_expr      (Inferer* inferer, Expr* expr, Type** type)          ;
bool inferer_resolve         (Inferer* inferer, Type* type, Type** resolved_type) ; // Takes a type, and attempts to find the bottom-most concrete type in the type graph.
bool inferer_attempt_unify   (Inferer* inferer, Type** left_ref, Type** right_ref); // Unifies the two types
bool inferer_unify           (Inferer* inferer, Type** left_ref, Type** right_ref); // Unifies the two types
bool inferer_generalize      (Inferer* inferer, Type* type, Type** scheme)        ;

// Follows free type variables until until we find a concrete type.
bool inferer_infer_expr_and_constrain(Inferer* inferer, Expr* expr, TypeConstraint* constraint, Type** type);

Type* inferer_create_free_type_var      (Inferer* inferer);
Type* inferer_create_free_list_type     (Inferer* inferer);
Type* inferer_create_free_function_type (Inferer* inferer , int arity);
Type* inferer_get_decl_var_type(Inferer* inferer, Decl* decl);
void  inferer_set_decl_var_type(Inferer* inferer, Decl* decl, Type* type);
Type* inferer_get_decl_var_return_type(Inferer* inferer, Decl* decl);
void  inferer_set_decl_var_return_type(Inferer* inferer, Decl* decl, Type* type);
Type* inferer_resolve_type_variable(Inferer* inferer, Type* var);
bool  inferer_occurs_check(Inferer* inferer, Type* var, Type* type);
bool  inferer_bind_variable_to_type(Inferer* inferer, Type* var, Type* type);
void  inferer_unify_apply_binds(Inferer* inferer);
void  inferer_unify_free_binds (Inferer* inferer);

// InfererSnapshot inferer_get_context_snapshot    (Inferer* inferer);
// void            inferer_restore_context_snapshot(Inferer* inferer, InfererSnapshot snapshot);
void assert_generic_operator_type_is_valid(TypeKind type);

void inferer_throw_compiler_error(Inferer* inferer, CompileError err);

#endif
