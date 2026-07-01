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
    DECL_LET  ,
    DECL_FN   ,
    DECL_ALIAS,
    DECL_TYPE ,
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

struct DeclAlias
{
    char* identifier;
    Type* type;
};

struct DeclType
{
    char* identifier;
    Type* type;
};

struct DeclTypeConstructor // Has to be declared after a 'DeclType'
{
    Variable* constructor;
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

    // Outputs
    Decl**    declarations;
    Expr**    exprs;
    char**    identifiers;

    // Errs
    CompileError** errs;
};

Resolver resolver_init(Parser parser, Stmt* stmt);
void resolver_free(Resolver* resolver);

#endif
