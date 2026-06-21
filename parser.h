#ifndef PARSER_H
#define PARSER_H

#include "dependencies.h"
#include "scanner.h"

typedef struct Parser       Parser      ;
typedef struct Stmt         Stmt        ;
typedef struct Type         Type        ;
typedef struct ExprOp       ExprOp      ;

typedef enum   ParserState  ParserState ;
typedef enum   StmtKind     StmtKind    ;
typedef enum   TypeKind     TypeKind    ;
typedef enum   ExprOpKind   ExprOpKind  ;

typedef struct StmtBlock    StmtBlock   ;
typedef struct StmtLet      StmtLet     ;
typedef struct StmtIf       StmtIf      ;
typedef struct StmtElif     StmtElif    ;
typedef struct StmtWhile    StmtWhile   ;
typedef struct StmtFnArgs   StmtFnArgs  ;
typedef struct StmtFn       StmtFn      ;

typedef struct TypeFunction TypeFunction;
typedef struct TypeStruct   TypeStruct  ;

// Parser
enum ParserState
{
    PARSER_STATE_UNPARSED,
    PARSER_STATE_PARSED  ,
};

struct Parser
{
    ParserState state ;
    const char* txt   ;

    Token* tokens;
    int start  ;
    int end    ;
    int current;

    // Arena
    Arena arena;

    // All of these have to be free at a later date.
    ExprOp* exprs; // Not used right now
    // char**  identifiers ;
    // char**  str_literals; // Not used right now
    // char**  int_literals; // Not used right now
    // char**  num_literals; // Not used right now

    // If we fail to parse something for whatever reason, we append an error message here.
    char** log;
};

// Stmt
enum StmtKind
{
    STMT_ERR              ,
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
        ExprOp   * expr      ;
        StmtIf   * if_stmt   ;
        StmtWhile* while_stmt;
        StmtFn   * fn        ;
    }
    stmt;
};

struct StmtBlock
{
    int   size ; // Number of statements
    Stmt* stmts;
};

struct StmtLet
{
    char  * identifier;
    Type  * type      ;
    ExprOp* expr      ;
};

struct StmtIf
{
    ExprOp  * condition;
    Stmt    * body     ;
    StmtElif* stmt_else; // else or elif
};

// I'm thinking that if the condition is NULL, then it's an 'else' statement.
// Otherwise, it's an 'elif' statement.
// Though this may be a bit fragile, so I'm not sure yet.
struct StmtElif
{
    ExprOp  * condition;
    Stmt    * body     ;
    StmtElif* stmt_else; // else or elif
};

struct StmtWhile
{
    ExprOp* condition;
    Stmt  * body     ;
};

struct StmtFn
{
    char*       identifier ;
    int         argc       ;
    StmtFnArgs* argv       ;
    Type*       return_type;
    StmtBlock*  body       ;
};

struct StmtFnArgs
{
    char*   identifier;
    Type*   type      ;
};

// Type
enum TypeKind
{
    // Special
    TYPE_UNKNOWN  ,

    TYPE_PRIMITIVE_SEPERATOR,

    // Built-in primitives
    // TYPE_TYPE     ,
    TYPE_NIL      ,
    TYPE_BOOL     ,
    TYPE_INT      , // i64 - for now at least
    TYPE_REAL     , // f64 - for now at least

    TYPE_DERIVATIVE_SEPERATOR,

    // Built-in derivative
    TYPE_LIST     ,
    TYPE_STRUCT   ,
    TYPE_FUNCTION ,

    TYPE_MISC_SEPERATOR,

    // TYPE_SEPERATOR, // Used to seperate the built-in types, from newly created ones
};

// Tagged Union
struct Type
{
    TypeKind kind;

    union
    {
        void        * primitive  ; // Primitives don't have any elements, so this would be NULL.
        TypeKind      list       ;
        TypeFunction* function   ;
        TypeStruct  * struct_type;
    }
    body;
};

struct TypeFunction
{
    int   argc;
    Type* argv;
    Type* return_type;
};

// Expr
enum ExprOpKind
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

struct ExprOp
{
    ExprOpKind op_type ;
    Type*      type    ;
    int        args    ;

    const char* literal;     // Points to parser literals (string, int, num, etc.)
    int line  ;
    int column;
    int length;
};

Parser init_parser(Scanner scanner);
Parser free_parser(Scanner scanner);

Token parser_peek(Parser* parser);
Token parser_next(Parser* parser);

// Stmt
Stmt     * parser_parse_stmt(Parser* parser);

StmtLet  * parser_parse_stmt_let(Parser* parser);
StmtIf   * parser_parse_stmt_if(Parser* parser);
StmtWhile* parser_parse_stmt_while(Parser* parser);
char     * parser_parse_identifier(Parser* parser);

// Type parsing
Type* parser_parse_type(Parser* parser);
Type* parser_parse_type_inner(Parser* parser);
// ExprOp* parser_parse_type(Parser* parser);
// ExprOp* parser_parse_type_inner(Parser* parser, int min_binding_power);

// Expr parsing
typedef enum
{
    LHS_OP_TYPE_ATOM  ,
    LHS_OP_TYPE_PREFIX,
    LHS_OP_TYPE_PARENS,
}
LhsOpType;
ExprOp* parser_parse_expr(Parser* parser);
ExprOp* parser_parse_expr_inner(Parser* parser, int min_binding_power);

void prefix_binding_power(ExprOpKind op_type, int* right);
bool postfix_binding_power(ExprOpKind op_type, int* left);
bool infix_binding_power(ExprOpKind op_type, int* left, int* right);
void append_rhs_to_expr(ExprOp** expr, ExprOp** rhs);

void print_expr_op(ExprOp* op);

#endif
