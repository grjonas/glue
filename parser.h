#ifndef PARSER_H
#define PARSER_H

#include "scanner.h"
#include "stmt.h"
#include "type_expr.h"
#include "expr.h"

typedef struct Parser       Parser      ;
typedef enum   ParserState  ParserState ;

// Parser
enum ParserState
{
    PARSER_STATE_UNPARSED,
    PARSER_STATE_PARSED  ,
    PARSER_STATE_FREED   ,
};

struct Parser
{
    // Input
    const char* filename;
    const char* txt   ;
    DYNAMIC_ARRAY(Token* tokens);

    // State
    int start  ;
    int end    ;
    int current;

    // Output
    Arena arena;
    DiagnosticComponent* diagnostic_component;
};

Parser init_parser(Scanner* scanner);
void parser_free(Parser* parser);

Token parser_peek_offset(Parser* parser, int offset);
Token parser_peek(Parser* parser);
Token parser_next(Parser* parser);

bool  parser_skip(Parser* parser, bool (*predicate)(TokenType));

char* copy_string_to_arena(Arena* arena, const char* str, int length);
char* parser_parse_identifier (Parser* parser);
bool  parser_accept_token(Parser* parser, TokenType type);
bool  parser_expect_token(Parser* parser, TokenType type);
bool  parser_dont_except_token(Parser* parser, TokenType type);

Span  parser_get_token_span(Parser* parser, Token token);
bool  is_newline(TokenType type);
Span  parser_combine_spans(Parser* parser, Span start, Span end);

void parser_throw_err_generic                     (Parser* parser, Token token, const char* file, int line);
void parser_throw_err_unexpected_token            (Parser* parser, Token token, TokenType expected[], int expected_count);
void parser_throw_err_expected_token              (Parser* parser, Token token, TokenType expected[], int expected_count);
void parser_throw_err_unexpected_prefix_operator  (Parser* parser, Token token);
void parser_throw_err_struct_duplicate_identifier (Parser* parser, Token identifier_token);
void parser_throw_err_unexpected_pattern          (Parser* parser, Span span);

#endif
