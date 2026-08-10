// Takes a stream of characters and outputs a stream of lexemes.
#ifndef SCANNER_H
#define SCANNER_H

#include "dependencies.h"
#include "token.h"
#include "diagnostic.h"

#define HANDLE_SCANNER_RESULT_BASE_CASE(result) \
    do \
    { \
        ScannerResult PRIVATE__result = (result); \
        if (PRIVATE__result != SCANNER_RESULT_RECOVERABLE_ERROR) \
        { \
            return PRIVATE__result; \
        } \
    } \
    while (false)

typedef struct ScannerStrView ScannerStrView;
typedef enum   ScannerResult  ScannerResult ;
typedef struct Scanner Scanner;

struct ScannerStrView
{
    int length;
    char* str ;
};

enum ScannerResult
{
    SCANNER_RESULT_SUCCESS            ,
    SCANNER_RESULT_RECOVERABLE_ERROR  ,
    SCANNER_RESULT_IRRECOVERABLE_ERROR,
};

struct Scanner
{
    const char* filename;
    char* init;
    const char* current;
    DYNAMIC_ARRAY(Token* token_list);

    int line;
    int column;

    DiagnosticComponent* diagnostic_component;
};

char* read_file(const char* filename);

Scanner init_scanner(const char* filename);
void free_scanner(Scanner* scanner);

bool scanner_is_at_end    (Scanner* scanner);
bool scanner_peek_char    (Scanner* scanner, char* char_ref);
bool scanner_consume_char (Scanner* scanner, char* char_ref);
bool scanner_accept_char  (Scanner* scanner, char charr);
bool scanner_expect_char  (Scanner* scanner, char charr);
bool scanner_peek_str     (Scanner* scanner, int length, const char** str_ref);
bool scanner_consume_str  (Scanner* scanner, int length, const char** str_ref);
bool scanner_accept_str   (Scanner* scanner, int length, const char* str);
bool scanner_expect_str   (Scanner* scanner, int length, const char* str);
void scanner_consume_until_inclusive(Scanner* scanner, bool (*predicate)(char));
void scanner_consume_until_exclusive(Scanner* scanner, bool (*predicate)(char));

void scanner_add_token(Scanner* scanner, Token token);
Token scanner_create_init_token(Scanner* scanner, TokenType);

ScannerResult scanner_scan_comment    (Scanner* scanner, Token* token_ref);
ScannerResult scanner_scan_str        (Scanner* scanner, Token* token_ref);
ScannerResult scanner_scan_number     (Scanner* scanner, Token* token_ref);
ScannerResult scanner_scan_identifier (Scanner* scanner, Token* token_ref);
ScannerResult scanner_scan_keyword    (Scanner* scanner, const char* keyword, TokenType token_type, Token* token_ref);
ScannerResult scanner_scan_keywords   (Scanner* scanner, Token* token_ref);
bool scanner_convert_identifier_to_keyword  (Scanner* scanner, const char* keyword, TokenType token_type, Token* token_ref);
bool scanner_convert_identifier_to_keywords (Scanner* scanner, Token* token_ref);

ScannerResult scanner_scan_token      (Scanner* scanner, Token* token_ref);

bool scanner_scan_tokens(Scanner* scanner);

bool is_char_newline (char charr);
bool is_digit        (char charr);
bool not_digit       (char charr);
bool is_alpha        (char charr);
bool is_whitespace   (char charr);
bool not_whitespace  (char charr);
bool is_identifier_start   (char charr);
bool is_identifier_middle  (char charr);
bool not_identifier_middle (char charr);
bool is_identifier_end     (char charr);

Span scanner_get_scanner_span(Scanner* scanner);

void scanner_throw_err_reached_eof         (Scanner* scanner);
void scanner_throw_err_unexpected_char     (Scanner* scanner, char charr);
void scanner_throw_err_unexpected_str      (Scanner* scanner, int length, const char* str);
void scanner_throw_err_str_non_terminating (Scanner* scanner);
void scanner_throw_err_expected_digit      (Scanner* scanner);

#endif
