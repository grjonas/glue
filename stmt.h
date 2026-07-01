#ifndef STMT_H
#define STMT_H

#include "scanner.h"
#include "parser.h"
#include "expr.h"

typedef struct Stmt         Stmt        ;

typedef enum   StmtKind     StmtKind    ;
typedef struct StmtBlock    StmtBlock   ;
typedef struct StmtLet      StmtLet     ;
typedef struct StmtIf       StmtIf      ;
typedef struct StmtElif     StmtElif    ;
typedef struct StmtWhile    StmtWhile   ;
typedef struct StmtFnArg    StmtFnArg   ;
typedef struct StmtFn       StmtFn      ;
typedef struct StmtReturn   StmtReturn  ;
typedef struct StmtAlias    StmtAlias   ;

// Stmt
enum StmtKind
{
    STMT_ERR              ,
    STMT_BLOCK            ,
    STMT_LET              ,
    STMT_EXPR             ,
    STMT_IF               ,
    // STMT_ELSE             ,
    STMT_WHILE            ,
    STMT_BREAK            ,
    STMT_CONTINUE         ,
    STMT_FN               ,
    STMT_RETURN           ,
    STMT_ALIAS            ,
};

struct StmtBlock
{
    int    size; // Number of statements
    Stmt** body;
};

struct StmtLet
{
    char     * identifier;
    TypeExpr * type ;
    Expr*      expr      ;
};

// I'm thinking that if the condition is NULL, then it's an 'else' statement.
// Otherwise, it's an 'elif' statement.
// Though this may be a bit fragile, so I'm not sure yet.
struct StmtIf
{
    Expr    * condition;
    Stmt    * body     ;
    Stmt    * next     ; // else or elif
};

struct StmtWhile
{
    Expr* condition;
    Stmt* body     ;
};

struct StmtFn
{
    char      * identifier ;
    int         argc       ;
    StmtFnArg** argv       ;
    TypeExpr  * return_type;
    Stmt      * body       ;
};

struct StmtFnArg
{
    char*     identifier;
    TypeExpr* type      ;
};

struct StmtReturn
{
    Expr* expr;
};

struct StmtAlias
{
    TypeExpr* type;
};

// Tagged Union
struct Stmt
{
    StmtKind kind;
    int line     ;
    int column   ;
    int length   ;

    // Can't have keywords as variables, so I just duplicate the last letter to get over that.
    union
    {
        Expr*      expr   ;
        StmtBlock  block  ;
        StmtLet    let    ;
        StmtIf     iff    ;
        StmtWhile  whilee ;
        StmtFn     fn     ;
        StmtReturn returnn;
        StmtAlias  alias  ;
        void*      none   ; // For statements that are just singular tokens such as 'break' or 'continue'.
    }
    stmt;
};

// Stmt
Stmt     * parser_parse_stmts        (Parser* parser);

Stmt     * parser_parse_stmt         (Parser* parser);

Stmt     * parser_parse_stmt_block   (Parser* parser);
Stmt     * parser_parse_stmt_let     (Parser* parser);
Stmt     * parser_parse_stmt_if      (Parser* parser);
Stmt     * parser_parse_stmt_while   (Parser* parser);
Stmt     * parser_parse_stmt_fn      (Parser* parser);
StmtFnArg* parser_parse_stmt_fn_arg  (Parser* parser);
Stmt     * parser_parse_stmt_expr    (Parser* parser);
Stmt     * parser_parse_stmt_break   (Parser* parser);
Stmt     * parser_parse_stmt_continue(Parser* parser);
Stmt     * parser_parse_stmt_return  (Parser* parser);
Stmt     * parser_parse_stmt_alias   (Parser* parser);

void print_stmt(Stmt* stmt);
const char* stmt_type_name(StmtKind kind);

#endif
