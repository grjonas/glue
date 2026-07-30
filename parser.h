#ifndef PARSER_H
#define PARSER_H

#include "dependencies.h"
#include "scanner.h"
#include "diagnostic.h"

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
    const char* txt   ;
    DYNAMIC_ARRAY(Token* tokens);

    // State
    ParserState state ;
    int start  ;
    int end    ;
    int current;

    // Output
    Arena arena;
    DiagnosticComponent* diagnostic_component;
};

Parser init_parser(Scanner scanner);
void parser_free(Parser* parser);

Token parser_peek(Parser* parser);
Token parser_next(Parser* parser);

Token parser_jump(Parser* parser, int new_state);
Token parser_restore(Parser* parser, int old_state);
bool  parser_skip(Parser* parser, bool (*predicate)(TokenType));

char* copy_string_to_arena(Arena* arena, const char* str, int length);
char* parser_parse_identifier (Parser* parser);
bool  parser_accept_token(Parser* parser, TokenType type);
bool  parser_expect_token(Parser* parser, TokenType type);
bool  parser_dont_except_token(Parser* parser, TokenType type);

void parser_throw_err_generic(Parser* parser, Token token, const char* file, int line);
void parser_throw_err_unexpected_token(Parser* parser, Token token, TokenType expected[], int expected_count);
void parser_throw_err_expected_token(Parser* parser, Token token, TokenType expected[], int expected_count);
void parser_throw_err_unexpected_prefix_operator(Parser* parser, Token token);
void parser_throw_err_struct_duplicate_identifier(Parser* parser, Token identifier_token);

#endif
