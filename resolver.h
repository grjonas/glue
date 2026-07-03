#ifndef RESOLVER_H
#define RESOLVER_H

#include "dependencies.h"
#include "stmt.h"
#include "expr.h"
#include "decl.h"

struct Resolver
{
    // Inputs
    const char* txt;
    Token* tokens;
    Stmt*  stmts;

    // Memory-management
    Arena  arena;
    Arena  tmp_type_arena;

    // Misc. state
    int loop_depth;
    bool inside_function;
    Type* fn_type;
    Decl** context; // Works similiar to a stack - when we recursively try to resolve a statement,
                    // we use this as context - on return, we restore the stack to it's previous state.

    // Outputs
    Decl** declarations; // Holds ALL scanned declarations
    Expr** exprs;
    char** identifiers;
    Type** types;

    // Errs
    CompileError** errs;
};

Resolver resolver_init(Parser parser, Stmt* stmt);
void resolver_free(Resolver* resolver);

void resolver_resolve_stmt(Resolver* resolver);
void resolver_resolve_expr(Resolver* resolver, Expr** expr);

Type* resolver_resolve_type_expr(Resolver* resolver, TypeExpr* type_expr);

int  resolver_get_context_snapshot(Resolver* resolver);
void resolver_restore_context_snapshot(Resolver* resolver, int snapshot);

void resolver_push_decl_to_context(Resolver* resolver, Decl* decl);
void resolve_push_expr(Resolver* resolver, Expr* expr);

Type* resolver_resolve_stmt_fn_type(Resolver* resolver, StmtFn fn);

Decl* resolver_declare_let(Resolver* resolver, char* identifier, Type* type);
Decl* resolver_declare_type_variable(Resolver* resolver, char* identifier);
Decl* resolver_get_decl_by_identifier(Resolver* resolver, char* identifier);

int   type_get_polymorphic_parameter_num(Type* type);

void resolver_throw_compiler_error(Resolver* resolver, CompileError err);

#endif
