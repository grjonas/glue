// Takes a stream of characters and outputs a stream of lexemes.
#ifndef SCANNER_H
#define SCANNER_H

#include "dependencies.h"

typedef enum
{
    // Single-character tokens.
    TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
    TOKEN_COMMA, TOKEN_MINUS, TOKEN_PLUS,
    TOKEN_SEMICOLON, TOKEN_STAR,
    TOKEN_NEWLINE,

    // One or two character tokens.
    TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER, TOKEN_GREATER_EQUAL,
    TOKEN_LESS, TOKEN_LESS_EQUAL,
    TOKEN_DOT, TOKEN_DOT_DOT,
    TOKEN_SLASH, TOKEN_SLASH_EQUAL,

    // Literals.
    TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER, TOKEN_COMMENT,

    // Keywords.
    TOKEN_AND, TOKEN_ELSE, TOKEN_ELIF, TOKEN_FALSE,
    TOKEN_FOR, TOKEN_FN, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
    TOKEN_PRINT, TOKEN_RETURN, TOKEN_THIS,
    TOKEN_TRUE, TOKEN_LET, TOKEN_WHILE, TOKEN_LOOP, TOKEN_MATCH,

    TOKEN_ERROR, TOKEN_EOF
}
TokenType;

const char* token_type_name(TokenType type);

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
    Token* token_list;

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

#endif
