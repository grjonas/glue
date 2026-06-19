#ifndef PARSER_H
#define PARSER_H

#include "dependencies.h"
#include "scanner.h"
/*
STMT_LET
PTR_TO_STR
TYPE
PTR_TO_EXPR
*/
#define ARENA_NULL 0
#define arrfree_and_set_null(op) do { arrfree(op); op = NULL; } while(0)

typedef enum
{
    STMT_ERR              ,
    STMT_LET_BARE         ,
    STMT_LET_TYPE         ,
    STMT_LET_EXPR         ,
    STMT_LET_TYPE_AND_EXPR,
    STMT_EXPR             ,
    STMT_IF               ,
    STMT_ELIF             ,
    STMT_ELSE             ,
    STMT_WHILE            ,
    STMT_BREAK            ,
    STMT_CONTINUE         ,
    STMT_FN               ,
    STMT_RETURN           ,
    STMT_EMPTY            ,
}
StmtType;

typedef union
{
    struct
    {
        StmtType type ;
        int32_t  depth;
    }
    header;
    struct
    {
        int32_t line  ;
        int32_t column;
    }
    position;
    int32_t length;
    size_t arena_ptr;
}
Stmt;

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
}
ExprOpType;

/*
*/
typedef enum
{
    TYPE_UNKNOWN,
    TYPE_NIL    ,
    TYPE_BOOL   ,
    TYPE_INT    , // i64 - for now at least
    TYPE_FLOAT  , // f64 - for now at least
}
ExprAtomicType;

typedef struct
{
    ExprOpType     op_type ;
    ExprAtomicType type    ;
    int32_t        args    ;

    const char* literal ;
    int32_t line  ;
    int32_t column;
    int32_t length;
}
ExprOp;

typedef struct
{
    const char* txt   ;
    Token*      tokens;

    Arena arena;

    int32_t start;
    int32_t end;
    int32_t current;
}
Parser;

Parser init_parser(Scanner scanner);
Parser free_parser(Scanner scanner);

Token parser_peek(Parser* parser);
Token parser_next(Parser* parser);

typedef enum
{
    LHS_OP_TYPE_ATOM  ,
    LHS_OP_TYPE_PREFIX,
    LHS_OP_TYPE_PARENS,
}
LhsOpType;

void stmt_append_header(Stmt* stmt, StmtType type, int32_t depth, int32_t line, int32_t column, int32_t length);
Stmt* parser_parse_stmts(Parser* parser, int32_t depth);
Stmt* parser_parse_stmt(Parser* parser, int32_t depth);
bool parser_skip(Parser* parser, bool (*predicate)(TokenType));
bool is_newline(TokenType type);
size_t parser_parse_identifier(Parser* parser);
size_t parser_parse_type(Parser* parser);
Stmt* parser_parse_block(Parser* parser, int32_t depth);

size_t parser_parse_expr(Parser* parser);
ExprOp* parser_parse_expr_inner(Parser* parser, int8_t min_binding_power);

void prefix_binding_power(ExprOpType op_type, int8_t* right);
bool postfix_binding_power(ExprOpType op_type, int8_t* left);
bool infix_binding_power(ExprOpType op_type, int8_t* left, int8_t* right);

void append_rhs_to_expr(ExprOp** expr, ExprOp** rhs);

//ExprOp* parser_parse_expr_inner(Parser parser, int8_t minimum_binding_power);

//void infix_binding_power(ExprOpType op, int8_t* left, int8_t* right);
//void prefix_binding_power(ExprOp op, int8_t* right);
void print_expr_op(ExprOp* op);

#endif
