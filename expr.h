#ifndef EXPR_H
#define EXPR_H

#include "parser.h"
#include "type.h"

typedef struct Expr            Expr           ;
typedef struct ExprPrimary     ExprPrimary    ;
typedef struct ExprUnary       ExprUnary      ;
typedef struct ExprBinary      ExprBinary     ;
typedef struct ExprFn          ExprFn         ;

typedef enum   ExprKind        ExprKind       ;
typedef enum   ExprPrimaryKind ExprPrimaryKind;
typedef enum   ExprUnaryKind   ExprUnaryKind  ;
typedef enum   ExprBinaryKind  ExprBinaryKind ;
// typedef enum   ExprFnKind      ExprFnKind     ;

enum ExprKind
{
    EXPR_PRIMARY,
    EXPR_UNARY  ,
    EXPR_BINARY ,
    EXPR_FN     ,
};

// TODO: Redo ExprPrimaryKind later.
enum ExprPrimaryKind
{
    EXPR_PRIMARY_UNKNOWN   ,
    EXPR_PRIMARY_NIL       ,
    EXPR_PRIMARY_BOOLEAN   ,
    EXPR_PRIMARY_STRING    ,
    EXPR_PRIMARY_NATURAL   ,
    EXPR_PRIMARY_INTEGER   ,
    EXPR_PRIMARY_REAL      ,
    EXPR_PRIMARY_STRUCT    ,
    EXPR_PRIMARY_FN        ,
    EXPR_PRIMARY_IDENTIFIER,
    EXPR_PRIMARY_PRINT     ,
};

enum ExprUnaryKind
{
    EXPR_UNARY_UNKNOWN       ,
    // EXPR_UNARY_PRE_INCREMENT ,
    // EXPR_UNARY_PRE_DECREMENT ,
    // EXPR_UNARY_POST_INCREMENT,
    // EXPR_UNARY_POST_DECREMENT,
    EXPR_UNARY_NOT           ,
    EXPR_UNARY_NEGATE        ,
};

enum ExprBinaryKind
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
};

struct ExprPrimary
{
    ExprPrimaryKind kind;
    union
    {
        // Nil is not included here
        char* nil       ; // Always NULL here.
        char* identifier;
        bool  boolean   ;
        char* string    ;
        char* natural   ; // These will have to be changed later i think.
        char* integer   ; // These will have to be changed later i think.
        char* real      ; // These will have to be changed later i think.
        char* obj       ; // Some kind of other object.
        // struct
    }
    primary;
};

struct ExprUnary
{
    ExprUnaryKind kind;
    Expr* unary;
};

struct ExprBinary
{
    ExprBinaryKind kind;
    Expr* left ;
    Expr* right;
};

struct ExprFn
{
    int    argc  ;
    Expr*  caller;
    Expr** argv  ;
};

struct Expr
{
    ExprKind kind;
    Type*    type;
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

// typedef enum
// {
//     LHS_OP_TYPE_ATOM  ,
//     LHS_OP_TYPE_PREFIX,
//     LHS_OP_TYPE_PARENS,
// }
// LhsOpType;
Expr* parser_parse_expr(Parser* parser);

Expr* parser_parse_expr_primary(Parser* parser);
Expr* parser_parse_expr_parens (Parser* parser);
Expr* parser_parse_expr_prefix (Parser* parser);
Expr* parser_parse_expr_index  (Parser* parser);
Expr* parser_parse_expr_fn     (Parser* parser);

ExprUnaryKind  get_prefix_operator(TokenType type, int* right_bp               );
ExprBinaryKind get_infix_operator (TokenType type, int* left_bp , int* right_bp);
ExprUnaryKind get_postfix_operator(TokenType type, int* right_bp               );

bool is_infix(TokenType type);
bool is_postfix(TokenType type, int* left_bp);

void print_expr_op(Expr* op);

#endif
