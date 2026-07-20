#ifndef RESOLVER_H
#define RESOLVER_H

#include "dependencies.h"
#include "stmt.h"
#include "expr.h"
#include "decl.h"

typedef struct Resolver Resolver;
typedef struct ResolverSnapshot ResolverSnapshot;

struct Resolver
{
    // Inputs
    const char* txt;
    DYNAMIC_ARRAY Token* tokens;
    Stmt*  stmts;

    // Memory-management
    Arena  arena;

    // Misc. state
    int decl_id;
    int type_variable_id;
    int loop_depth;
    bool inside_function;
    DYNAMIC_ARRAY Decl** context; // Works similiar to a stack - when we recursively try to resolve a statement,
                    // we use this as context - on return, we restore the stack to it's previous state.

    // Outputs
    DYNAMIC_ARRAY Decl** declarations; // Holds ALL scanned declarations
    DYNAMIC_ARRAY char** identifiers;

    // Errs
    DYNAMIC_ARRAY CompileError** errs;
};

struct ResolverSnapshot
{
    int context_length  ;
    // int type_variable_id;
    // int decl_id;
};

Resolver resolver_init(Parser* parser, Stmt* stmt);
void resolver_free(Resolver* resolver);

// 'true' indicates success, 'false' indicates failure.
bool resolver_resolve_stmt     (Resolver* resolver);
bool resolver_resolve_expr     (Resolver* resolver, Expr* expr);
bool resolver_resolve_type_expr(Resolver* resolver, TypeExpr* type_expr);

ResolverSnapshot resolver_get_context_snapshot    (Resolver* resolver);
void     resolver_restore_context_snapshot(Resolver* resolver, ResolverSnapshot snapshot);

void resolver_push_decl_to_context(Resolver* resolver, Decl* decl);

Decl* resolver_declare_variable            (Resolver* resolver, char* identifier);
Decl* resolver_declare_type_variable       (Resolver* resolver, char* identifier);
Decl* resolver_declare_alias               (Resolver* resolver, char* identifier);
Decl* resolver_declare_new_type            (Resolver* resolver, char* identifier);
Decl* resolver_declare_new_type_constructor(Resolver* resolver, char* identifier);

char* resolver_get_existing_identifier(Resolver* resolver, char* identifier);
Decl* resolver_get_decl_by_identifier(Resolver* resolver, char* identifier);

void resolver_throw_compiler_error(Resolver* resolver, CompileError err);

// resolver_declare_type_variable
// resolver_resolve_stmt_fn_type
// resolver_declare_fn_params
#endif
