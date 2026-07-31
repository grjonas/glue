#include "parser.h"

#define arrfree_and_set_null(op) do { arrfree(op); op = NULL; } while(0)

Parser init_parser(Scanner scanner)
{
    Parser parser =
    {
        .filename     = scanner.filename          ,
        .state        = PARSER_STATE_UNPARSED     ,
        .txt          = scanner.init              ,
        .tokens       = scanner.token_list        ,
        .start        = 0                         ,
        .end          = arrlen(scanner.token_list),
        .current      = 0                         ,
        .diagnostic_component = diagnostic_component_init(),
    };

    return parser;
}

void parser_free(Parser* parser)
{
    free((char*) parser->txt);
    arrfree(parser->tokens);
    arena_free(&parser->arena);
    // assert(parser->log == NULL); // Idk, placed it here just in case.
    diagnostic_component_free(&parser->diagnostic_component);

    *parser = (Parser)
    {
        .filename = NULL              ,
        .state    = PARSER_STATE_FREED,
        .txt      = NULL              ,
        .tokens   = NULL              ,
        .start    = -1                ,
        .end      = -1                ,
        .current  = -1                ,
        .diagnostic_component = NULL  ,
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
    if (!parser_expect_token(parser, TOKEN_IDENTIFIER))
    {
        return NULL;
    }
    identifier = copy_string_to_arena(&parser->arena, token.start, token.length);

    return identifier;
}

bool parser_accept_token(Parser* parser, TokenType type)
{
    Token token = parser_peek(parser);
    if (token.type != type)
    {
        return false;
    }

    parser_next(parser);
    return true;
}

bool parser_expect_token(Parser* parser, TokenType type)
{
    Token token = parser_peek(parser);
    if (!parser_accept_token(parser, type))
    {
        TokenType expected[] =
        {
            type
        };
        parser_throw_err_unexpected_token(parser, token, expected, 1);
        return false;
    }

    return true;
}

bool parser_dont_except_token(Parser* parser, TokenType type)
{
    Token token = parser_peek(parser);
    if (parser_accept_token(parser, type))
    {
        TokenType expected[] =
        {
            type
        };
        parser_throw_err_expected_token(parser, token, expected, 1);
        return false;
    }

    return true;
};

Span parser_get_token_span(Parser* parser, Token token)
{
    assert(parser != NULL);

    return (Span)
    {
        .filename = parser->filename,
        .line     = token.line      ,
        .column   = token.column    ,
        .length   = token.length    ,
    };
}

void parser_throw_err_generic(Parser* parser, Token token, const char* file, int line)
{
    assert(parser != NULL);

    diagnostic_component_push_err(parser->diagnostic_component, (DiagnosticErr)
    {
        .kind = DIAGNOSTIC_ERR_GENERIC,
        .span = parser_get_token_span(parser, token),
        .err.generic =
        {
            .file = file,
            .line = line,
        }
    });
}

void parser_throw_err_unexpected_token(Parser* parser, Token token, TokenType expected[], int expected_count)
{
    assert(parser != NULL);

    diagnostic_component_push_err(parser->diagnostic_component, (DiagnosticErr)
    {
        .kind = DIAGNOSTIC_ERR_UNEXPECTED_TOKEN,
        .span = parser_get_token_span(parser, token),
        .err.unexpected_token =
        {
            .expected = (TokenType*) arena_push
                (&parser->diagnostic_component->arena, expected, sizeof(TokenType) * expected_count),
            .expected_count = expected_count,
        }
    });
}

void parser_throw_err_expected_token(Parser* parser, Token token, TokenType expected[], int expected_count)
{
    assert(parser != NULL);

    diagnostic_component_push_err(parser->diagnostic_component, (DiagnosticErr)
    {
        .kind = DIAGNOSTIC_ERR_EXPECTED_TOKEN,
        .span = parser_get_token_span(parser, token),
        .err.expected_token =
        {
            .expected = (TokenType*) arena_push
                (&parser->diagnostic_component->arena, expected, sizeof(TokenType) * expected_count),
            .expected_count = expected_count,
        }
    });
}

void parser_throw_err_unexpected_prefix_operator(Parser* parser, Token token)
{
    assert(parser != NULL);

    diagnostic_component_push_err(parser->diagnostic_component, (DiagnosticErr)
    {
        .kind = DIAGNOSTIC_ERR_UNEXPECTED_PREFIX_OP,
        .span = parser_get_token_span(parser, token),
        .err.unexpected_prefix_op =
        {
            .token_type = token.type,
        }
    });
}

void parser_throw_err_struct_duplicate_identifier(Parser* parser, Token identifier_token)
{
    assert(parser != NULL);
    assert(identifier_token.type == TOKEN_IDENTIFIER);

    diagnostic_component_push_err(parser->diagnostic_component, (DiagnosticErr)
    {
        .kind = DIAGNOSTIC_ERR_STRUCT_DUPLICATE_IDENTIFIER,
        .span = parser_get_token_span(parser, identifier_token),
        .err.struct_duplicate_identifier =
        {
            .identifier = diagnostic_component_add_identifier
                (parser->diagnostic_component, identifier_token.start, identifier_token.length)
        }
    });
}
