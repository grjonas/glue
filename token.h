#ifndef TOKEN_H
#define TOKEN_H

#include "dependencies.h"

typedef enum   TokenType TokenType;
typedef struct Token     Token    ;

enum TokenType
{
    // Single-character tokens.
    TOKEN_LEFT_PAREN , TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE , TOKEN_RIGHT_BRACE,
    TOKEN_LEFT_SQUARE, TOKEN_RIGHT_SQUARE,
    TOKEN_PIPE,
    TOKEN_COMMA,
    TOKEN_SEMICOLON,
    TOKEN_UNDERSCORE,
    TOKEN_NEWLINE,

    // One or two character tokens.
    TOKEN_BANG      , TOKEN_BANG_EQUAL   ,
    TOKEN_EQUAL     , TOKEN_EQUAL_EQUAL  , TOKEN_EQUAL_GREATER,
    TOKEN_LESS      , TOKEN_LESS_EQUAL   ,
    TOKEN_GREATER   , TOKEN_GREATER_EQUAL,
    TOKEN_DOT       , TOKEN_DOT_DOT      ,
    TOKEN_COLON     , TOKEN_COLON_COLON  ,
    TOKEN_PLUS      , TOKEN_PLUS_EQUAL   , TOKEN_PLUS_PLUS    ,
    TOKEN_MINUS     , TOKEN_MINUS_EQUAL  , TOKEN_MINUS_MINUS  , TOKEN_MINUS_GREATER,
    TOKEN_STAR      , TOKEN_STAR_EQUAL   ,
    TOKEN_SLASH     , TOKEN_SLASH_EQUAL  ,
    TOKEN_PERCENT   , TOKEN_PERCENT_EQUAL,

    // Literals.
    TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_INTEGER, TOKEN_NUMBER, TOKEN_COMMENT,

    // Keywords.
    TOKEN_NIL_T, TOKEN_BOOL  , TOKEN_NAT   , TOKEN_INT   , TOKEN_REAL  ,
    TOKEN_LET  , TOKEN_ALIAS , TOKEN_TYPE  , TOKEN_NIL_V ,
    TOKEN_TRUE , TOKEN_FALSE , TOKEN_NOT   , TOKEN_AND   , TOKEN_OR    ,
    TOKEN_DO   , TOKEN_END   ,
    TOKEN_IF   , TOKEN_ELIF  , TOKEN_ELSE  ,
    TOKEN_WHILE, TOKEN_FOR   , TOKEN_IN    , TOKEN_BREAK , TOKEN_LOOP  , TOKEN_CONTINUE,
    TOKEN_FN   , TOKEN_RETURN,
    TOKEN_MATCH,

    // Special.
    TOKEN_WHITESPACE, TOKEN_ERROR, TOKEN_EOF
};

    //Given T is a structure type: struct { TK key; TV value; }. Note that some
struct Token
{
    TokenType type;
    const char* start;
    int32_t line;
    int32_t column;
    int32_t length;
};

#endif
