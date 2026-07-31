// Takes a stream of characters and outputs a stream of lexemes.
#ifndef SCANNER_H
#define SCANNER_H

#include "dependencies.h"
#include "token.h"

typedef struct Scanner   Scanner  ;

struct Scanner
{
    const char* filename;
    char* init;
    const char* start;
    const char* current;
    DYNAMIC_ARRAY(Token* token_list);

    int32_t line;
    int32_t column;
};

const char* token_type_name(TokenType type);
char* read_file(const char* filename);

Scanner init_scanner(const char* filename);
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
