#ifndef RESOLVER_H
#define RESOLVER_H

#include "dependencies.h"
#include "stmt.h"
#include "expr.h"
#include "decl.h"

typedef struct Resolver Resolver;
typedef struct Snapshot Snapshot;

struct Resolver
{
    // Inputs
    const char* txt;
    Token* tokens;
    Stmt*  stmts;

    // Memory-management
    Arena  arena;

    // Misc. state
    int decl_id;
    int type_variable_id;
    int loop_depth;
    bool inside_function;
    Decl** context; // Works similiar to a stack - when we recursively try to resolve a statement,
                    // we use this as context - on return, we restore the stack to it's previous state.

    // Outputs
    Decl** declarations; // Holds ALL scanned declarations
    char** identifiers;

    // Errs
    CompileError** errs;
};

struct Snapshot
{
    int context_length  ;
    // int type_variable_id;
    // int decl_id;
};

Resolver resolver_init(Parser* parser, Stmt* stmt);
void resolver_free(Resolver* resolver);

void resolver_resolve_stmt(Resolver* resolver);
void resolver_resolve_expr(Resolver* resolver, Expr** expr);

Type* resolver_resolve_type_expr(Resolver* resolver, TypeExpr* type_expr);

Snapshot  resolver_get_context_snapshot(Resolver* resolver);
void resolver_restore_context_snapshot(Resolver* resolver, Snapshot snapshot);

void resolver_push_decl_to_context(Resolver* resolver, Decl* decl);

Type* resolver_resolve_stmt_fn_type(Resolver* resolver, StmtFn fn);
Type* resolver_create_type_variable(Resolver* resolver);

Decl* resolver_declare_let(Resolver* resolver, char* identifier, Type* type);
Decl* resolver_declare_type_variable(Resolver* resolver, char* identifier);
Decl* resolver_get_decl_by_identifier(Resolver* resolver, char* identifier);
void resolver_declare_fn_params(Resolver* resolver, StmtFn fn, Type* fn_type);
void decl_set_type(Decl* decl, Type* type);

int   type_get_polymorphic_parameter_num(Type* type);

void resolver_throw_compiler_error(Resolver* resolver, CompileError err);

#endif
