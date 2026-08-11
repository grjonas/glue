#ifndef PARSER_H
#define PARSER_H

#include "scanner.h"

// Parser
typedef enum
{
    PARSER_STATE_UNPARSED,
    PARSER_STATE_PARSED  ,
    PARSER_STATE_FREED   ,
}
ParserState;

typedef struct
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
}
Parser;

// Parsing building blocks:

extern Parser init_parser(Scanner* scanner);
extern void parser_free(Parser* parser);

extern Token parser_peek_offset(Parser* parser, int offset);
extern Token parser_peek(Parser* parser);
extern Token parser_next(Parser* parser);
extern bool  parser_skip(Parser* parser, bool (*predicate)(TokenType));
extern char* parser_parse_identifier (Parser* parser);
extern bool  parser_accept_token(Parser* parser, TokenType type);
extern bool  parser_expect_token(Parser* parser, TokenType type);
extern bool  parser_dont_except_token(Parser* parser, TokenType type);

extern char* copy_string_to_arena(Arena* arena, const char* str, int length);

extern Span  parser_get_token_span(Parser* parser, Token token);
extern bool  is_newline(TokenType type);
extern Span  parser_combine_spans(Parser* parser, Span start, Span end);

extern void parser_throw_err_generic                     (Parser* parser, Token token, const char* file, int line);
extern void parser_throw_err_unexpected_token            (Parser* parser, Token token, TokenType expected[], int expected_count);
extern void parser_throw_err_expected_token              (Parser* parser, Token token, TokenType expected[], int expected_count);
extern void parser_throw_err_unexpected_prefix_operator  (Parser* parser, Token token);
extern void parser_throw_err_struct_duplicate_identifier (Parser* parser, Token identifier_token);
extern void parser_throw_err_unexpected_pattern          (Parser* parser, Span span);

// Major parsers:

extern Stmt * parser_parse_stmts(Parser* parser);
extern Stmt* parser_parse_stmt(Parser* parser);
extern Expr* parser_parse_expr(Parser* parser);
extern TypeExpr* parser_parse_type_expr(Parser* parser);
extern bool parser_parse_pattern  (Parser* parser, Pattern** pattern_ref);
extern FnArg* parser_parse_fn_arg(Parser* parser);

#endif
