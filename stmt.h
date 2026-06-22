#ifndef STMT_H
#define STMT_H

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

// Stmt
enum StmtKind
{
    STMT_ERR              ,
    STMT_BLOCK            ,
    STMT_LET              ,
    STMT_EXPR             ,
    STMT_IF               ,
    STMT_ELIF             ,
    // STMT_ELSE             ,
    STMT_WHILE            ,
    STMT_BREAK            ,
    STMT_CONTINUE         ,
    STMT_FN               ,
    STMT_RETURN           ,
    STMT_EMPTY            ,
};

// Tagged Union
struct Stmt
{
    StmtKind kind;
    int line     ;
    int column   ;
    int length   ;

    union
    {
        StmtBlock* block     ;
        StmtLet  * let       ;
        Expr   * expr      ;
        StmtIf   * if_stmt   ;
        StmtWhile* while_stmt;
        StmtFn   * fn        ;
    }
    stmt;
};

struct StmtBlock
{
    int    size; // Number of statements
    Stmt** body;
};

struct StmtLet
{
    char  * identifier;
    Type  * type      ;
    Expr* expr      ;
};

struct StmtIf
{
    Expr  * condition;
    Stmt    * body     ;
    StmtElif* stmt_else; // else or elif
};

// I'm thinking that if the condition is NULL, then it's an 'else' statement.
// Otherwise, it's an 'elif' statement.
// Though this may be a bit fragile, so I'm not sure yet.
struct StmtElif
{
    Expr  * condition;
    Stmt    * body     ;
    StmtElif* stmt_else; // else or elif
};

struct StmtWhile
{
    Expr* condition;
    Stmt  * body     ;
};

// TODO: Refactor this slightly such that we have a function Type field.
struct StmtFn
{
    char*       identifier ;
    int         argc       ;
    StmtFnArg** argv       ;
    Type*       return_type;
    Stmt     *  body       ;
};

struct StmtFnArg
{
    char*   identifier;
    Type*   type      ;
};

// Stmt
Stmt     * parser_parse_stmt       (Parser* parser);

StmtBlock* parser_parse_stmt_block (Parser* parser);
StmtLet  * parser_parse_stmt_let   (Parser* parser);
StmtIf   * parser_parse_stmt_if    (Parser* parser);
StmtWhile* parser_parse_stmt_while (Parser* parser);
StmtFn   * parser_parse_stmt_fn    (Parser* parser);
StmtFnArg* parser_parse_stmt_fn_arg(Parser* parser);
char     * parser_parse_identifier (Parser* parser);

void print_stmt(Stmt* stmt);
const char* stmt_type_name(StmtKind kind);

#endif
