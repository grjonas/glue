#ifndef PARSER_H
#define PARSER_H

#include "dependencies.h"
#include "scanner.h"

//typedef enum
//{
//    // Linear
//    STMT_LET,
//    STMT_ASSIGN,
//    STMT_EXPR
//    // Conditional
//    STMT_IF,
//    // Loops
//    STMT_WHILE,
//    STMT_LOOP,
//    STMT_BREAK,
//    STMT_CONTINUE,
//    // Functions
//    STMT_FN,
//    STMT_RETURN,
//    // Types
//    STMT_TYPE,
//    STMT_MATCH,
//    // Effects
//    STMT_EFFECT,
//    STMT_HANDLE,
//    STMT_ERROR,
//}
//StmtType;
//
//typedef void StmtInfo;
//
//typedef struct
//{
//    expr
//}
//StmtExpr;
//
//typedef struct
//{
//    StmtType type;
//    int32_t start;
//    int32_t end;
//    StmtInfo* info; // StmtIf, StmtWhile, etc.
//}
//Stmt;

typedef enum
{
    EXPR_TYPE_UNKNOWN
}
ExprType;

typedef enum
{
    // Atom
    OP_STRING,
    OP_IDENTIFIER,
    OP_INTEGER,
    OP_NUMBER,
    OP_TRUE,
    OP_FALSE,
    OP_NIL,
    // Unary
    OP_NEG, // unary '-'
    OP_NOT,
    OP_INDEX,
    // Binary
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
    // Unique
    OP_CALL,
    //OP_COLON, // technically binary, but it has different behaviour
}
ExprOpType;

//#define IS_EXPR_OP(e) ( IS_ATOM(e) || IS_OPERATOR(e) )

typedef struct
{
    ExprOpType  op_type ;
    int32_t     args    ;
    ExprType    type    ;
    const char* literal ; // for function or variable names
    int32_t literal_size;
    int32_t line;
    int32_t column;
    int32_t length;
}
ExprOp;

typedef struct
{
    int32_t start;
    int32_t end;
    int32_t current;
    //Stmt*   stmts;
}
ParserInfo;

typedef struct
{
    const char* txt;
    Token* tokens;
    ParserInfo info;
}
Parser;

Parser init_parser(Scanner scanner);
Parser free_parser(Scanner scanner);

//Parser parser_parse_let(Parser parser);

ExprOp* parser_parse_expr(Parser parser);
ExprOp* parser_parse_expr_inner(Parser parser, int8_t minimum_binding_power);

void infix_binding_power(ExprOpType op, int8_t* left, int8_t* right);
void prefix_binding_power(ExprOp op, int8_t* right);

#endif
