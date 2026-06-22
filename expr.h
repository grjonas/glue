#ifndef EXPR_H
#define EXPR_H

#include "parser.h"
#include "type.h"

typedef struct Expr            Expr           ;
typedef struct ExprOperand     ExprOperand    ;
typedef struct ExprUnary       ExprUnary      ;
typedef struct ExprBinary      ExprBinary     ;
typedef struct ExprFn          ExprFn         ;

typedef enum   ExprKind        ExprKind       ;
typedef enum   ExprOperandKind ExprOperandKind;
typedef enum   ExprUnaryKind   ExprUnaryKind  ;
typedef enum   ExprBinaryKind  ExprBinaryKind ;
// typedef enum   ExprFnKind      ExprFnKind     ;

// Expr
enum ExprKind
{
    // Atom
    OP_STRING,
    OP_IDENTIFIER,
    OP_INTEGER,
    OP_NUMBER,
    OP_TRUE,
    OP_FALSE,
    OP_NIL,
    OP_PRINT,
    // Prefix
    OP_NEG, // unary '-'
    OP_NOT,
    OP_PRE_INC,
    OP_PRE_DEC,
    // Postfix
    OP_INDEX,
    OP_POST_INC,
    OP_POST_DEC,
    // Infix
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_AND,
    OP_OR ,
    OP_GREATER,
    OP_LESS,
    OP_GREATER_EQUAL,
    OP_LESS_EQUAL,
    OP_EQUAL,
    OP_NOT_EQUAL,
    OP_ACCESS,
    OP_CHAIN,
    // Assign (right associative)
    OP_ASSIGN,
    OP_ASSIGN_ADD,
    OP_ASSIGN_SUB,
    OP_ASSIGN_MUL,
    OP_ASSIGN_MOD,
    // Unique
    OP_CALL,
    //OP_COLON, // technically binary, but it has different behaviour
};

// TODO: Redo ExprOperandKind later.
enum ExprOperandKind
{
    EXPR_OPERAND_BOOLEAN   ,
    EXPR_OPERAND_STRING    ,
    EXPR_OPERAND_NATURAL   ,
    EXPR_OPERAND_INTEGER   ,
    EXPR_OPERAND_REAL      ,
    EXPR_OPERAND_STRUCT    ,
    EXPR_OPERAND_FN        ,
    EXPR_OPERAND_IDENTIFIER,
    EXPR_OPERAND_PRINT     ,
};

enum ExprUnaryKind
{
    EXPR_UNARY_PRE_INCREMENT ,
    EXPR_UNARY_PRE_DECREMENT ,
    EXPR_UNARY_POST_INCREMENT,
    EXPR_UNARY_POST_DECREMENT,
    EXPR_UNARY_NOT           ,
    EXPR_UNARY_NEGATE        ,
};

enum ExprBinaryKind
{
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
    // TODO: Add more types of assignment
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
        ExprOperand* operand;
        ExprUnary  * unary  ;
        ExprBinary * binary ;
        ExprFn     * fn     ;
    }
    stmt;
};

struct ExprOperand
{
    ExprOperandKind kind;
    union
    {
        // Nil is not included here
        char* identifier;
        bool  boolean   ;
        char* string    ;
        char* natural   ; // These will have to be changed later i think.
        char  integer   ; // These will have to be changed later i think.
        char* real      ; // These will have to be changed later i think.
        char* obj       ; // Some kind of other object.
        // struct
    }
    operand;
};

struct ExprUnary
{
    ExprUnaryKind kind;
    Expr* expr;
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
// typedef enum
// {
//     LHS_OP_TYPE_ATOM  ,
//     LHS_OP_TYPE_PREFIX,
//     LHS_OP_TYPE_PARENS,
// }
// LhsOpType;
Expr* parser_parse_expr(Parser* parser);

void prefix_binding_power(ExprKind op_type, int* right);
bool postfix_binding_power(ExprKind op_type, int* left);
bool infix_binding_power(ExprKind op_type, int* left, int* right);
void append_rhs_to_expr(Expr** expr, Expr** rhs);

void print_expr_op(Expr* op);

#endif
