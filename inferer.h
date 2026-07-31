#ifndef INFERER_H
#define INFERER_H

#include "resolver.h"
#include "type.h"

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
    Type* var ;
    Type* type;
};

struct TypeEnv
{
    int type_variable_length;
};

struct Subst
{
    Type* key;
    int   value;
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
    DYNAMIC_ARRAY(Type** type_variables); // A stack of type variables
    DYNAMIC_ARRAY(Bind * binds);
    TypeEnv curr_type_env;

    // Outputs

    // Errs
    DiagnosticComponent* diagnostic_component;
};

Inferer inferer_init(Resolver* resolver);
void    inferer_free(Inferer* inferer  );

bool inferer_infer_stmt          (Inferer* inferer, Stmt* stmt)                         ;
void inferer_convert_type_expr   (Inferer* inferer, TypeExpr* type_expr, Type** type)   ;
bool inferer_infer_expr          (Inferer* inferer, Expr* expr, Type** type)            ;
bool inferer_resolve             (Inferer* inferer, Type* type, Type** resolved_type)   ; // Takes a type, and attempts to find the bottom-most concrete type in the type graph.
bool inferer_unify_inner         (Inferer* inferer, Type** left_ref, Type** right_ref)  ; // Unifies the two types
bool inferer_unify               (Inferer* inferer, Type** left_ref, Type** right_ref, Span span); // Unifies the two types
bool inferer_attempt_unify       (Inferer* inferer, Type** left_ref, Type** right_ref)  ;
void inferer_generalize          (Inferer* inferer, Type* type, TypeScheme** scheme)    ;
void inferer_instantiate         (Inferer* inferer, TypeScheme* scheme, Type** type)    ;
void inferer_get_substs          (Inferer* inferer, Type* type, HASHMAP(Subst*)* substs);
void inferer_apply_subst         (Inferer* inferer, Type* type, Subst subst)            ;
void inferer_apply_subst_reverse (Inferer* inferer, Type* type, Subst subst)            ;

// Follows free type variables until until we find a concrete type.
bool inferer_infer_expr_and_constrain(Inferer* inferer, Expr* expr, TypeConstraint* constraint, Type** type);

Type* inferer_create_free_type_var      (Inferer* inferer);
Type* inferer_create_free_list_type     (Inferer* inferer);
Type* inferer_create_free_function_type (Inferer* inferer, int arity);
Type* inferer_create_list_type          (Inferer* inferer, Type* inferred_type);

void  inferer_decl_var_begin_inferrence    (Inferer* inferer, Decl* decl);
Type* inferer_decl_var_get_type            (Inferer* inferer, Decl* decl);
void  inferer_decl_var_set_type            (Inferer* inferer, Decl* decl, Type* type);
Type* inferer_decl_var_get_return_type     (Inferer* inferer, Decl* decl);
void  inferer_decl_var_set_return_type     (Inferer* inferer, Decl* decl, Type* type);
Type* inferer_decl_type_var_get_type       (Inferer* inferer, Decl* decl);
void  inferer_decl_type_var_set_type       (Inferer* inferer, Decl* decl, Type* type);
Type* inferer_decl_alias_get_type          (Inferer* inferer, Decl* decl);
void  inferer_decl_alias_set_type          (Inferer* inferer, Decl* decl, Type* type);

void  inferer_decl_var_generalize_inferred (Inferer* inferer, Decl* decl);
TypeScheme* inferer_decl_var_get_scheme    (Inferer* inferer, Decl* decl);
void        inferer_decl_var_set_scheme    (Inferer* inferer, Decl* decl, TypeScheme* scheme);

TypeAbstraction* inferer_create_type_abstraction(Inferer* inferer, int type_var_num);
TypeAbstraction* inferer_get_existing_new_type_from_decl(Inferer* inferer, Decl* decl);
TypeAbstraction* inferer_decl_type_get_abstraction(Inferer* inferer, Decl* decl);
void             inferer_decl_type_set_abstraction(Inferer* inferer, Decl* decl, TypeAbstraction* abstraction);

void  inferer_push_type_variable(Inferer* inferer, Type* type_var);
void  inferer_pop_type_variable (Inferer* inferer);
Type* inferer_resolve_type_variable(Inferer* inferer, Type* var);
bool  inferer_occurs_check(Inferer* inferer, Type* var, Type* type);
bool  inferer_bind_variable_to_type(Inferer* inferer, Type* var, Type* type);
void  inferer_unify_apply_binds(Inferer* inferer);
void  inferer_unify_free_binds (Inferer* inferer);

TypeEnv inferer_get_curr_type_env(Inferer* inferer);
void    inferer_set_curr_type_env(Inferer* inferer, TypeEnv type_env);
void assert_generic_operator_type_is_valid(TypeKind type);
bool inferer_type_applications_are_equal(Inferer* inferer, TypeApplication left_application, TypeApplication right_application);

Span inferer_get_expr_span(Inferer* inferer, Expr* expr);

void inferer_throw_err_unify_failed                                        (Inferer* inferer, Span span, Type* left, Type* right);
void inferer_throw_err_type_failed_constraint_numeric                      (Inferer* inferer, Span  span);
void inferer_throw_err_type_failed_constraint_equality                     (Inferer* inferer, Span  span);
void inferer_throw_err_expr_binary_arithmetic_constraint_failed            (Inferer* inferer, Expr* expr);
void inferer_throw_err_expr_binary_equality_constraint_failed              (Inferer* inferer, Expr* expr);
void inferer_throw_err_expr_binary_access_op_left_kind_not_struct          (Inferer* inferer, Expr* expr);
void inferer_throw_err_expr_binary_access_op_struct_does_not_contain_field (Inferer* inferer, Expr* expr);

#endif
