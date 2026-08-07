#ifndef STMT_H
#define STMT_H

#include "expr.h"
#include "ast_definitions.h"
#include "fn_arg.h"

typedef enum   StmtKind            StmtKind           ;
typedef struct StmtBlock           StmtBlock          ;
typedef struct StmtLet             StmtLet            ;
typedef struct StmtIf              StmtIf             ;
typedef struct StmtElif            StmtElif           ;
typedef struct StmtWhile           StmtWhile          ;
typedef struct StmtFn              StmtFn             ;
typedef struct StmtReturn          StmtReturn         ;
typedef struct StmtAlias           StmtAlias          ;
typedef struct StmtType            StmtType           ;
typedef struct StmtTypeConstructor StmtTypeConstructor;

// Stmt
enum StmtKind
{
    STMT_BLOCK            ,
    STMT_LET              ,
    STMT_EXPR             ,
    STMT_IF               ,
    STMT_WHILE            ,
    STMT_BREAK            ,
    STMT_CONTINUE         ,
    STMT_FN               ,
    STMT_RETURN           ,
    STMT_ALIAS            ,
    STMT_TYPE             ,
};

struct StmtBlock
{
    int    size; // Number of statements
    Stmt** body;
};

struct StmtLet
{
    char     * identifier;
    Decl     * decl      ;
    TypeExpr * type      ;
    Expr     * expr      ;
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
    Decl      * decl       ;
    TypeExpr  * return_type;
    Stmt      * body       ;
    FnArg    ** argv       ;
    int         argc       ;
};

struct StmtReturn
{
    Expr* expr;
    Decl* decl; // Function to which the return is tied to.
};

struct StmtAlias
{
    char    * identifier;
    TypeExpr* type      ;
    Decl    * decl      ;
};

struct StmtTypeConstructor
{
    char* identifier;
    TypeExpr** types;
    int type_num;
};

struct StmtType
{
    char               *  identifier     ;
    char               ** argv           ;
    StmtTypeConstructor** constructors   ;
    int                   argc           ;
    int                   constructor_num;
    Decl*                 decl           ;
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
        StmtType   type   ;
        void*      none   ; // For statements that are just singular tokens such as 'break' or 'continue'.
    }
    stmt;
};

#endif
