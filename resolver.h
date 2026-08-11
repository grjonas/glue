#ifndef RESOLVER_H
#define RESOLVER_H

#include "parser.h"

typedef struct Resolver Resolver;
typedef struct ResolverSnapshot ResolverSnapshot;

struct Resolver
{
    // Inputs
    const char* filename;
    const char* txt;
    DYNAMIC_ARRAY(Token* tokens);
    Stmt*  stmts;

    // Memory-management
    Arena  arena;

    // Misc. state
    int decl_id;
    int type_variable_id;
    int loop_depth;
    Decl* curr_fn; // NULL means not inside any function
    DYNAMIC_ARRAY(Decl** context); // Works similiar to a stack - when we recursively try to resolve a statement,
                    // we use this as context - on return, we restore the stack to it's previous state.

    // Outputs
    DYNAMIC_ARRAY(Decl** declarations); // Holds ALL scanned declarations
    DYNAMIC_ARRAY(char** identifiers );

    // Errs
    DiagnosticComponent* diagnostic_component;
};

struct ResolverSnapshot
{
    int context_length  ;
    // int type_variable_id;
    // int decl_id;
};

extern Resolver resolver_init(Parser* parser, Stmt* stmt);
extern void resolver_free(Resolver* resolver);

// 'true' indicates success, 'false' indicates failure.
extern bool resolver_resolve_stmt     (Resolver* resolver);
extern bool resolver_resolve_expr     (Resolver* resolver, Expr* expr);
extern bool resolver_resolve_type_expr(Resolver* resolver, TypeExpr* type_expr);
extern bool resolver_resolve_pattern  (Resolver* resolver, Pattern* pattern);
extern bool resolver_resolve_fn_args  (Resolver* resolver, int argc, FnArg** argv, TypeExpr* return_type);

#endif
