#ifndef PARSER_H
#define PARSER_H

#include "dependencies.h"
#include "scanner.h"
#include "compile_error.h"

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
    Token* tokens;

    // State
    ParserState state ;
    int start  ;
    int end    ;
    int current;

    // Output
    Arena arena;
    CompileError** errs;
};

Parser init_parser(Scanner scanner);
void parser_free(Parser* parser);

Token parser_peek(Parser* parser);
Token parser_next(Parser* parser);

Token parser_jump(Parser* parser, int new_state);
Token parser_restore(Parser* parser, int old_state);
bool parser_skip(Parser* parser, bool (*predicate)(TokenType));
char     * parser_parse_identifier (Parser* parser);

void parser_throw_compiler_error(Parser* parser, CompileError err);

#endif
