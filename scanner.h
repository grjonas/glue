// Takes a stream of characters and outputs a stream of lexemes.
#ifndef SCANNER_H
#define SCANNER_H

#include "dependencies.h"

typedef enum
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
}
TokenType;

const char* token_type_name(TokenType type);

    //Given T is a structure type: struct { TK key; TV value; }. Note that some
typedef struct
{
    TokenType type;
    const char* start;
    int32_t line;
    int32_t column;
    int32_t length;
}
Token;

typedef struct
{
    char* init;
    const char* start;
    const char* current;
    DYNAMIC_ARRAY Token* token_list;

    int32_t line;
    int32_t column;
}
Scanner;

char* read_file(const char* filename);

Scanner init_scanner(char* txt);
void free_scanner(Scanner* scanner);

bool scanner_is_at_end(Scanner scanner);
char scanner_peek(Scanner scanner);
char scanner_consume(Scanner* scanner);
bool scanner_match(Scanner* scanner, char to_match);
void scanner_skip_whitespace(Scanner* scanner);

Token scanner_scan_token(Scanner* scanner);

void scanner_add_token(Scanner* scanner, Token token);
Token scanner_make_token(Scanner* scanner, TokenType token_type, int32_t lines_to_skip, int32_t columns_to_skip);
Token scanner_make_error_token(Scanner scanner, const char* err_msg);

void scanner_scan_tokens(Scanner* scanner);
Token scanner_scan_string(Scanner* scanner);
Token scanner_scan_line_comment(Scanner* scanner);

bool is_digit(char c);
bool is_alpha(char c);
Token scanner_scan_number(Scanner* scanner);

bool scanner_match_string(Scanner* scanner, const char* str, int32_t already_scanned);
bool is_identifier_middle(char c);
bool is_identifier_end(char c);
Token scanner_scan_identifier(Scanner* scanner);

bool is_newline(TokenType type);

#endif
