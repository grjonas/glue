#ifndef PARSER_H
#define PARSER_H

#include "dependencies.h"
#include "scanner.h"

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
    ParserState state ;
    const char* txt   ;

    Token* tokens;
    int start  ;
    int end    ;
    int current;

    // Arena
    Arena arena;

    // All of these have to be free at a later date.
    // Expr* exprs; // Not used right now
    // char**  identifiers ;
    // char**  str_literals; // Not used right now
    // char**  int_literals; // Not used right now
    // char**  num_literals; // Not used right now

    // If we fail to parse something for whatever reason, we append an error message here.
    char** log;
};

Parser init_parser(Scanner scanner);
void parser_free(Parser* parser);

Token parser_peek(Parser* parser);
Token parser_next(Parser* parser);

Token parser_jump(Parser* parser, int new_state);
Token parser_restore(Parser* parser, int old_state);
bool parser_skip(Parser* parser, bool (*predicate)(TokenType));

#endif
