#include "parser_pattern.h"

void parser_attempt_parse_pattern_wildcard(Parser* parser, Pattern** pattern_ref)
{
    assert(parser != NULL);
    assert(pattern_ref  != NULL);
    assert(*pattern_ref == NULL);

    Token token = parser_next(parser);

    assert(token.type == TOKEN_UNDERSCORE);

    *pattern_ref = create_pattern(&parser->arena);
    **pattern_ref = (Pattern)
    {
        .kind   = PATTERN_WILDCARD,
        .line   = token.line      ,
        .column = token.column    ,
        .length = token.length    ,
    };
}

void parser_attempt_parse_pattern_literal(Parser* parser, Pattern** pattern_ref)
{
    assert(parser != NULL);
    assert(pattern_ref  != NULL);
    assert(*pattern_ref == NULL);

    Token token;
    PatternLiteral tmp_literal;

    token = parser_next(parser);
    switch (token.type)
    {
        case TOKEN_NIL_V     : tmp_literal.kind = PATTERN_LITERAL_NIL  ; break;
        case TOKEN_TRUE      : tmp_literal.kind = PATTERN_LITERAL_TRUE ; break;
        case TOKEN_FALSE     : tmp_literal.kind = PATTERN_LITERAL_FALSE; break;
        case TOKEN_NUMBER    :
            tmp_literal.kind = PATTERN_LITERAL_NUMBER ;
            tmp_literal.literal.number  = copy_literal_to_const_c_str
                (&parser->arena, token.length, token.start);
            break;

        case TOKEN_INTEGER   :
            tmp_literal.kind = PATTERN_LITERAL_INTEGER; break;
            tmp_literal.literal.integer = copy_literal_to_const_c_str
                (&parser->arena, token.length, token.start);
            break;

        case TOKEN_STRING    :
            tmp_literal.kind = PATTERN_LITERAL_STRING ; break;
            tmp_literal.literal.string  = copy_literal_to_const_c_str
                (&parser->arena, token.length, token.start);
            break;

        default: UNREACHABLE;
    }

    *pattern_ref = create_pattern(&parser->arena);
    **pattern_ref = (Pattern)
    {
        .kind   = PATTERN_LITERAL,
        .line   = token.line     ,
        .column = token.column   ,
        .length = token.length   ,
        .pattern.literal = tmp_literal
    };
}

void parser_attempt_parse_pattern_var(Parser* parser, Pattern** pattern_ref)
{
    assert(parser != NULL);
    assert(pattern_ref  != NULL);
    assert(*pattern_ref == NULL);

    Token token;
    PatternVar tmp_var;

    token = parser_next(parser);
    assert(token.type == TOKEN_IDENTIFIER);

    tmp_var = (PatternVar)
        { .var = copy_literal_to_const_c_str
            (&parser->arena, token.length, token.start) };

    *pattern_ref = create_pattern(&parser->arena);
    **pattern_ref = (Pattern)
    {
        .kind   = PATTERN_VAR    ,
        .line   = token.line     ,
        .column = token.column   ,
        .length = token.length   ,
        .pattern.var = tmp_var
    };
}

bool parser_attempt_parse_pattern_constructor(Parser* parser, Span* span_ref, Pattern** pattern_ref)
{
    assert(parser != NULL);
    assert(pattern_ref  != NULL);
    assert(*pattern_ref == NULL);

    Token token;
    Span start;
    Span end  ;
    Span combined;

    PatternConstructor tmp_constructor;
    const char* identifier = NULL;
    int   identifier_length = 0;
    Pattern** argv = NULL;
    int argc = 0;

    token = parser_next(parser);
    assert(token.type == TOKEN_IDENTIFIER);
    start = parser_get_token_span(parser, token);
    identifier = token.start;
    identifier_length = token.length;

    token = parser_peek(parser);
    if (!parser_accept_token(parser, TOKEN_LEFT_PAREN))
    {
        *span_ref = parser_get_token_span(parser, token);
        return false;
    }

    token = parser_peek(parser);
    if (!parser_accept_token(parser, TOKEN_RIGHT_PAREN))
    {
        Pattern* pattern = NULL;
        Span fail_span;

        do
        {
            pattern = NULL;

            if (!parser_attempt_parse_pattern(parser, &fail_span, &pattern))
            {
                *span_ref = fail_span;
                return false;
            }

            token = parser_peek(parser);
            if (token.type != TOKEN_COMMA && token.type != TOKEN_RIGHT_PAREN)
            {
                *span_ref = parser_get_token_span(parser, token);
                return false;
            }
            parser_next(parser);

            arrput(argv, pattern);
        }
        while (token.type != TOKEN_RIGHT_PAREN);

        end = parser_get_token_span(parser, token);

        DYNAMIC_ARRAY(Pattern**) tmp_argv = argv;
        argc = arrlen(argv);
        argv = (Pattern**) arena_push(&parser->arena, argv, argc * sizeof(Pattern*));
        arrfree(tmp_argv);
    }
    else
    {
        end = parser_get_token_span(parser, token);
    }

    combined = parser_combine_spans(parser, start, end);

    tmp_constructor = (PatternConstructor)
    {
        .identifier = copy_literal_to_const_c_str
            (&parser->arena, identifier_length, identifier),
        .argv = argv,
        .argc = argc,
    };

    *pattern_ref = create_pattern(&parser->arena);
    **pattern_ref = (Pattern)
    {
        .kind   = PATTERN_CONSTRUCTOR,
        .line   = combined.line      ,
        .column = combined.column    ,
        .length = combined.length    ,
        .pattern.constructor = tmp_constructor
    };
    return true;
}

// NOTE: This is written in a different style to the other parsing functions
// This is because those old parsing functions were written while I was
// still experimenting with different styles of coding.
// However, that style began to change as I wrote the resolver, and then
// fully crystalized (atleast as of me writing this) into the style you see here.
// I plan to eventually rewrite the parser code to fit with the new style,
// which is why this function uses the new style.
bool parser_attempt_parse_pattern(Parser* parser, Span* span_ref, Pattern** pattern_ref)
{
    assert(parser != NULL);
    assert(pattern_ref  != NULL);
    assert(*pattern_ref == NULL);

    Token token;
 
    token = parser_peek(parser);
    switch (token.type)
    {
        case TOKEN_UNDERSCORE: parser_attempt_parse_pattern_wildcard(parser, pattern_ref); return true;

        case TOKEN_NIL_V     :
        case TOKEN_TRUE      :
        case TOKEN_FALSE     :
        case TOKEN_NUMBER    :
        case TOKEN_INTEGER   :
        case TOKEN_STRING    :
            parser_attempt_parse_pattern_literal(parser, pattern_ref); return true;

        case TOKEN_IDENTIFIER:
            token = parser_peek_offset(parser, 1);
            if (token.type == TOKEN_LEFT_PAREN)
            {
                if (!parser_attempt_parse_pattern_constructor(parser, span_ref, pattern_ref))
                {
                    return false;
                }
            }
            else
            {
                parser_attempt_parse_pattern_var(parser, pattern_ref);
            }
            return true;

        default: *span_ref = parser_get_token_span(parser, token); return false;
    }
}

bool parser_parse_pattern(Parser* parser, Pattern** pattern_ref)
{
    assert(parser != NULL);
    assert(pattern_ref  != NULL);
    assert(*pattern_ref == NULL);

    Span span;

    if (!parser_attempt_parse_pattern(parser, &span, pattern_ref))
    {
        parser_throw_err_unexpected_pattern(parser, span);
        return false;
    }

    return true;
}

const char* copy_literal_to_const_c_str(Arena* arena, int length, const char* str)
{
    assert(arena != NULL);
    assert(str != NULL || length == 0);

    char* c_str = NULL;

    c_str = (char*) arena_push_empty(arena, sizeof(char) * (length + 1));
    memcpy(c_str, str, sizeof(char) * length);

    return (const char*) c_str;
}
