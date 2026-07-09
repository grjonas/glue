#ifndef INFERER_H
#define INFERER_H

#include "resolver.h"
#include "type.h"

// TODO: Don't forget to refactor inferer init and free after changing resolver.
// TODO: Think about rewriting substitution to use hashmaps instead of lists.
typedef struct Inferer       Inferer      ;

typedef struct Scheme        Scheme       ;
typedef struct TypeEnvTerm   TypeEnvTerm  ;
typedef enum   TypeAllocKind TypeAllocKind;
typedef struct SubstObj      SubstObj     ;
typedef struct Subst         Subst        ;
typedef struct TypeAlloc     TypeAlloc    ;

// A bonded type and it's associated variable ids.
struct Scheme
{
    Type* type;
    int*  bound_var_ids;
};

struct TypeEnvTerm
{
    int    key  ;
    Scheme value;
};

enum TypeAllocKind
{
    TYPE_ALLOC_TYPE    ,
    TYPE_ALLOC_SCHEME  ,
    TYPE_ALLOC_SUBST   ,
    TYPE_ALLOC_TYPE_ENV,
};

struct Subst
{
    int   key  ; // var id
    Type* value;
};

// A single object
struct TypeAlloc
{
    int reference_num; // How many objects hold reference to an object of this type.
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

    // Outputs

    // Errs
    CompileError** errs;
};

Inferer inferer_init(Resolver* resolver);
void    inferer_free(Inferer* inferer  );

void type_get_free_type_vars(Type type, int** free_type_vars);
void scheme_get_free_type_vars(Scheme scheme, int** free_type_vars);
void type_env_get_free_type_vars(TypeEnvTerm* env, int** free_type_vars);
Type* type_deepcopy(Arena* arena, Type* type);
void substitute_var_free_with_bounded(Type* type, int free_var, int bounded_var);
void inferer_type_apply_substitution(Inferer* inferer, Subst* subst_hashmap, Type** type_ref);
void generalize_type(TypeEnvTerm* env, Type* type, Scheme* scheme);
bool instantiate_scheme(Scheme scheme, Type* type_ref);
bool inferer_throw_failed_to_unify_error(Inferer* inferer);
Subst* compose_substitutions(Subst* subst_hashmap_a, Subst* subst_hashmap_b);
bool inferer_bind_variable_to_type(Inferer* inferer, int var_id, Type* type, Subst** subst);
bool inferer_get_most_general_unifier(Inferer* inferer, Type* left, Type* right, Subst** subst_hashmap);

void inferer_throw_compiler_error(Inferer* inferer, CompileError err);

#endif
