#ifndef RESOLVER_H
#define RESOLVER_H

#include "dependencies.h"
#include "stmt.h"
#include "expr.h"

typedef struct Resolver Resolver;

typedef enum   DeclKind DeclKind;

typedef struct DeclLet  DeclLet ;
typedef struct DeclFn   DeclFn  ;
typedef struct Decl     Decl    ;

enum DeclKind
{
    DECL_LET,
    DECL_FN ,
};

struct DeclLet
{
    Variable* variable;
};

struct DeclFn
{
    Variable* variable  ;
    int argc;
    Variable** argv;
};

// Declaration
struct Decl
{
    DeclKind kind;
    struct
    {
        DeclLet let;
        DeclFn  fn ;
    }
    decl;
};

struct Resolver
{
    // Inputs (And outputs)
    const char* txt;
    Token* tokens;
    Stmt*  stmts;

    Arena  arena;
    Arena  tmp_type_arena;

    int loop_depth;
    bool inside_function;
    Type* fn_type;

    Decl**    declarations;
    Expr**    exprs;
    Type**    types;
    char**    identifiers;

    CompileError** errs;
};

Resolver resolver_init(Parser parser, Stmt* stmt);
void resolver_free(Resolver* resolver);

Stmt* resolver_resolve_stmt(Resolver* resolver);
void  resolver_resolve_expr(Resolver* resolver, Expr* expr, Type* type);

Decl*  resolver_declare_let  (Resolver* resolver, char* identifier, Type* type);
Decl*  resolver_declare_fn   (Resolver* resolver, StmtFn fn);

char* resolver_get_identifier(Resolver* resolver, char* identifier);
Variable* resolver_get_nearest_variable(Resolver* resolver, char* identifier);

void resolver_throw_compiler_error(Resolver* resolver, CompileError err);

Type* construct_fn_type(Arena* arena, StmtFn fn);

#endif
