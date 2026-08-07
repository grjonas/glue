#include "parser_definitions.h"

FnArg* parser_parse_fn_arg(Parser* parser)
{
    FnArg  fn_arg;

    Token token;
    char* identifier = NULL;
    TypeExpr* type   = NULL;

    fn_arg = (FnArg)
    {
        .identifier = NULL,
        .decl       = NULL,
        .type       = NULL,
    };

    identifier = parser_parse_identifier(parser);
    if (identifier == NULL)
    {
        return NULL;
    }
    fn_arg.identifier = identifier;

    token = parser_peek(parser);
    if (token.type == TOKEN_COLON)
    {
        parser_next(parser);

        type = parser_parse_type_expr(parser);
        if (type == NULL)
        {
            return NULL;
        }
        fn_arg.type = type;
    }

    return (FnArg*) arena_push(&parser->arena, &fn_arg, sizeof(FnArg));
}

