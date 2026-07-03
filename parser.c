#include "parser.h"

#define arrfree_and_set_null(op) do { arrfree(op); op = NULL; } while(0)

Parser init_parser(Scanner scanner)
{
    Parser parser =
    {
        .state        = PARSER_STATE_UNPARSED     ,
        .txt          = scanner.init              ,
        .tokens       = scanner.token_list        ,
        .start        = 0                         ,
        .end          = arrlen(scanner.token_list),
        .current      = 0                         ,
        .errs         = NULL                      ,
    };

    return parser;
}

void parser_free(Parser* parser)
{
    free((char*) parser->txt);
    arrfree(parser->tokens);
    arena_free(&parser->arena);
    // assert(parser->log == NULL); // Idk, placed it here just in case.
    arrfree(parser->errs);

    *parser = (Parser)
    {
        .state   = PARSER_STATE_FREED   ,
        .txt     = NULL                 ,
        .tokens  = NULL                 ,
        .start   = -1                   ,
        .end     = -1                   ,
        .current = -1                   ,
        .errs    = NULL                 ,
    };
}

Token parser_peek(Parser* parser)
{
    const char* err = "No more tokens left.";
    if (parser->current >= parser->end)
    {
        return (Token)
        {
            .type   = TOKEN_ERROR,
            .start  = err,
            .line   = -1,
            .column = -1,
            .length = strlen(err)
        };
    }
    return parser->tokens[parser->current];
}

Token parser_next(Parser* parser)
{
    Token token = parser_peek(parser);
    if (token.type != TOKEN_ERROR)
        parser->current++;
    return token;
}

Token parser_jump(Parser* parser, int new_state)
{
    if (new_state < parser->start || parser->state < new_state)
    {
        fprintf(stderr, "[%s:%d] Statement parsing: Cannot jump out of bounds.\n", __FILE__, __LINE__);
        exit(1);
    }

    parser->current = new_state;
    return parser_peek(parser);
}

Token parser_restore(Parser* parser, int old_state)
{
    if (parser->current < old_state)
    {
        fprintf(stderr, "[%s:%d] Statement parsing: Cannot restore state to new state.\n", __FILE__, __LINE__);
        exit(1);
    }

    return parser_jump(parser, old_state);
}

bool parser_skip(Parser* parser, bool (*predicate)(TokenType))
{
    bool ret = false;
    while (true)
    {
        Token token = parser_peek(parser);
        if (predicate(token.type))
        {
            parser_next(parser);
            ret = true;
        }
        else
            break;
    }
    return ret;
}

char* copy_string_to_arena(Arena* arena, const char* str, int length)
{
    char* new_str = NULL;
    char* tmp_ptr = NULL;

    new_str = calloc(length + 1, sizeof(char));
    if (new_str  == NULL)
    {
        fprintf(stderr, "[%s:%d] Failed to allocate memory.\n", __FILE__, __LINE__);
        exit(1);
    }

    memcpy(new_str, str, (size_t) length * sizeof(char));

    tmp_ptr = new_str;
    new_str = (char*) arena_push(arena, new_str, (size_t) (length + 1) * sizeof(char));
    free(tmp_ptr);

    return new_str;
}

// Identifier
char* parser_parse_identifier(Parser* parser)
{
    char* identifier = NULL;
    Token token;

    token = parser_peek(parser);
    parser_expect_token(parser, TOKEN_IDENTIFIER);
    identifier = copy_string_to_arena(&parser->arena, token.start, token.length);

    return identifier;
}

bool parser_expect_token(Parser* parser, TokenType type)
{
    Token token = parser_peek(parser);
    if (token.type != type)
    {
        parser_throw_compiler_error(parser, (CompileError)
        {
            .kind   = ERROR_ERROR ,
            .line   = token.line  ,
            .column = token.column,
            .length = token.line  ,
            .msg    = "Unexpected token encountered",
        });
        return false;
    }

    parser_next(parser);
    return true;
}

void parser_throw_compiler_error(Parser* parser, CompileError err)
{
    CompileError* err_ptr = NULL;
    err_ptr = (CompileError*) arena_push(&parser->arena, &err, sizeof(CompileError));
    arrput(parser->errs, err_ptr);
}
