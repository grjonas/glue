#ifndef EXPR_H
#define EXPR_H

#include "type_expr.h"
#include "token.h"
#include "decl.h"
#include "stmt.h"
#include "fn_arg.h"

// TODO: Refactor 'expr.h' and 'expr.c' to make it cleaner.
// Also, replace the 'parser_throw_err_generic' functions as part of a larger rewrite.

typedef enum
{
    EXPR_PRIMARY,
    EXPR_UNARY  ,
    EXPR_BINARY ,
    EXPR_FN     ,
}
ExprKind;

typedef enum
{
    EXPR_PRIMARY_UNKNOWN   ,
    EXPR_PRIMARY_NIL       ,
    EXPR_PRIMARY_BOOLEAN   ,
    EXPR_PRIMARY_STRING    ,
    EXPR_PRIMARY_NATURAL   ,
    EXPR_PRIMARY_INTEGER   ,
    EXPR_PRIMARY_REAL      ,
    EXPR_PRIMARY_LIST      ,
    EXPR_PRIMARY_STRUCT    ,
    EXPR_PRIMARY_LAMBDA    ,
    EXPR_PRIMARY_IDENTIFIER,
    EXPR_PRIMARY_DECL      ,
    // EXPR_PRIMARY_VARIABLE  ,
}
ExprPrimaryKind;

typedef enum
{
    EXPR_UNARY_UNKNOWN       ,
    // EXPR_UNARY_PRE_INCREMENT ,
    // EXPR_UNARY_PRE_DECREMENT ,
    // EXPR_UNARY_POST_INCREMENT,
    // EXPR_UNARY_POST_DECREMENT,
    EXPR_UNARY_NOT           ,
    EXPR_UNARY_NEGATE        ,
}
ExprUnaryKind;

typedef enum
{
    EXPR_BINARY_UNKNOWN      ,
    EXPR_BINARY_ADD          ,
    EXPR_BINARY_SUBTRACT     ,
    EXPR_BINARY_MULTIPLY     ,
    EXPR_BINARY_DIVIDE       ,
    EXPR_BINARY_MODULO       ,
    EXPR_BINARY_AND          ,
    EXPR_BINARY_OR           ,
    EXPR_BINARY_EQUAL        ,
    EXPR_BINARY_NOT_EQUAL    ,
    EXPR_BINARY_LESS_EQUAL   ,
    EXPR_BINARY_LESS         ,
    EXPR_BINARY_GREATER_EQUAL,
    EXPR_BINARY_GREATER      ,
    EXPR_BINARY_CHAIN        ,
    EXPR_BINARY_ACCESS       ,
    EXPR_BINARY_ASSIGN       ,
    EXPR_BINARY_INDEX        ,
    // TODO: Add more types of assignment
}
ExprBinaryKind;

typedef struct
{
    char    * key  ;
    TypeExpr* type ;
    Expr    * value;
}
ExprPrimaryStructField;

typedef struct
{
    int    length;
    Expr** list;
}
ExprPrimaryList;

typedef struct
{
    int argc;
    ExprPrimaryStructField** argv;
}
ExprPrimaryStruct;

typedef struct
{
    Decl      * decl       ;
    TypeExpr  * return_type;
    Stmt      * body       ;
    FnArg    ** argv       ;
    int         argc       ;
}
ExprPrimaryLambda;

typedef struct
{
    ExprPrimaryKind kind;
    union
    {
        // Nil is not included here
        char* nil                ; // Always NULL here.
        char* identifier         ;
        bool  boolean            ;
        char* string             ;
        char* natural            ; // These will have to be changed later i think.
        char* integer            ; // These will have to be changed later i think.
        char* real               ; // These will have to be changed later i think.
        ExprPrimaryList   list   ;
        ExprPrimaryStruct structt;
        ExprPrimaryLambda lambda ;
        char* obj                ; // Some kind of other object.
        Decl* decl               ;
    }
    primary;
}
ExprPrimary;

typedef struct
{
    ExprUnaryKind kind;
    Expr* unary;
}
ExprUnary;

typedef struct
{
    ExprBinaryKind kind;
    Expr* left ;
    Expr* right;
}
ExprBinary;

typedef struct
{
    int    argc  ;
    Expr*  caller;
    Expr** argv  ;
}
ExprFn;

struct Expr
{
    ExprKind kind;
    int line  ;
    int column;
    int length;

    union
    {
        ExprPrimary primary;
        ExprUnary   unary  ;
        ExprBinary  binary ;
        ExprFn      fn     ;
    }
    expr;
};

ExprUnaryKind  get_prefix_operator(TokenType type, int* right_bp               );
ExprBinaryKind get_infix_operator (TokenType type, int* left_bp , int* right_bp);
ExprUnaryKind get_postfix_operator(TokenType type, int* right_bp               );

bool is_infix(TokenType type);
bool is_postfix(TokenType type, int* left_bp);

Expr** create_new_argument_list(Arena* arena, int old_argc, Expr** expr, Expr* lhs);

Expr* construct_assign_expr(Arena* arena, char* identifier, Expr* expr);

void print_expr_op(Expr* op);

#endif
