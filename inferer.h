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

// A type, and
struct Scheme
{
    Type*  type ;
    int bounded_var_num; // Number of bounded variables
};

struct TypeEnvTerm
{
    Scheme value;
    int    key  ;
};

struct TypeEnv
{
    TypeEnvTerm* terms;
};

enum TypeAllocKind
{
    TYPE_ALLOC_TYPE    ,
    TYPE_ALLOC_SCHEME  ,
    TYPE_ALLOC_SUBST   ,
    TYPE_ALLOC_TYPE_ENV,
};

struct SubstObj
{
    int   key  ; // free var id
    Type* value;
};

struct Subst
{
    SubstObj* substs; // Allocated using stb
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
    Arena  arena;

    // Misc. state

    // Outputs

    // Errs
    CompileError** errs;
};

Inferer inferer_init(Resolver* resolver);
void    inferer_free(Inferer* inferer  );

#endif
