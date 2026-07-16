#ifndef INFERER_H
#define INFERER_H

#include "resolver.h"
#include "type.h"

#define BUILTIN_TYPE_NIL    NULL
#define BUILTIN_TYPE_BOOL   NULL
#define BUILTIN_TYPE_STRING NULL
#define BUILTIN_TYPE_NAT    NULL
#define BUILTIN_TYPE_INT    NULL
#define BUILTIN_TYPE_REAL   NULL

// TODO: Don't forget to refactor inferer init and free after changing resolver.
typedef enum   InfererReturn InfererReturn;
typedef struct Inferer       Inferer      ;
typedef struct TypeScheme    TypeScheme   ;

enum InfererReturn
{
    INFERER_RETURN_ERROR       ,
    INFERER_RETURN_SUCCESS     ,
    INFERER_RETURN_UNIFY_FAILED,
};

struct TypeScheme
{
    int quantified_count;
    Type* type;
};

// There are a couple of things that should be known about the inferer:
// 1) Unlike some other components, we benefit from not using a arenas as religiously as we did before.
//     Instead, we should have a 'Subst**', which would allow us to deallocate, and reallocate memory at will.
struct Inferer
{
    // Inputs
    const char* txt;
    Token* tokens;
    Stmt*  stmts;
    Decl** declarations; // Holds ALL scanned declarations
    char** identifiers;

    // Memory-management
    Arena arena;
    Arena type_arena;

    // Misc. state
    Type** type_variables;

    // Outputs

    // Errs
    CompileError** errs;
};

Inferer inferer_init(Resolver* resolver);
void    inferer_free(Inferer* inferer  );

bool inferer_infer_expr(Inferer* inferer, Expr* expr, Type** type);
bool inferer_resolve(Inferer* inferer, Type* type, Type** resolved_type); // Takes a type, and attempts to find the bottom-most concrete type in the type graph.
bool inferer_unify     (Inferer* inferer, Type** left, Type** right);     // Unifies the two types
bool inferer_infer_and_unify_with_type_of_type_kind(Inferer* inferer, Expr* expr, TypeKind expr_type, Type** type);

// Follows free type variables until until we find a concrete type.
Type* inferer_resolve_type_variable(Inferer* inferer, Type* type_var);

void assert_generic_operator_type_is_valid(TypeKind type);

void inferer_throw_compiler_error(Inferer* inferer, CompileError err);

#endif
