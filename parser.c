#include "parser.h"

Parser init_parser(Scanner scanner)
{
    Parser parser =
    {
        .txt = scanner.init,
        .tokens = scanner.token_list,
        .start    = 0,
        .end      = arrlen(scanner.token_list),
        .current  = 0
    };

    return parser;
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

#define MAKE_OP_FROM_TOKEN(token, oper_type, args_num, expr_type) (ExprOp){\
        .op_type = (oper_type),\
        .args = (args_num),\
        .type = (expr_type),\
        .literal = (token).start,\
        .line = (token).line,\
        .column = (token).column,\
        .length = (token).length\
    } 
ExprOp* parser_parse_expr(Parser* parser)
{
    ExprOp* expr = NULL;
    Token token;

    ExprOp lhs, op, rhs;

    // Parse first literal
    token = parser_next(parser);
    switch (token.type)
    {
        case TOKEN_NUMBER : lhs = MAKE_OP_FROM_TOKEN(token, OP_NUMBER , 0, TYPE_UNKNOWN); break;
        case TOKEN_INTEGER: lhs = MAKE_OP_FROM_TOKEN(token, OP_INTEGER, 0, TYPE_UNKNOWN); break;
        //case TOKEN_IDENTIFIER: expr = MAKE_OP_FROM_TOKEN(token, OP_IDENTIFIER, 0, TYPE_UNKNOWN); break;
        default:
            fprintf(stderr, "Parser line %d: Bad token found.\n", __LINE__);
    }
    
    // Parse binary operator

    token = parser_peek(parser);
    switch (token.type)
    {
        case TOKEN_PLUS : op = MAKE_OP_FROM_TOKEN(token, OP_ADD, 2, TYPE_UNKNOWN); break;
        case TOKEN_ERROR: arrput(expr, lhs); return expr;
        default:
            fprintf(stderr, "Parser line %d: Bad token found.\n", __LINE__);
    }
    parser_next(parser);

    // Parse second literal
    token = parser_next(parser);
    switch (token.type)
    {
        case TOKEN_NUMBER : rhs = MAKE_OP_FROM_TOKEN(token, OP_NUMBER , 0, TYPE_UNKNOWN); break;
        case TOKEN_INTEGER: rhs = MAKE_OP_FROM_TOKEN(token, OP_INTEGER, 0, TYPE_UNKNOWN); break;
        //case TOKEN_IDENTIFIER: expr = MAKE_OP_FROM_TOKEN(token, OP_IDENTIFIER, 0, TYPE_UNKNOWN); break;
        default:
            fprintf(stderr, "Parser line %d: Bad token found.\n", __LINE__);
    }

    arrput(expr, op );
    arrput(expr, lhs);
    arrput(expr, rhs);
    return expr;
}

//    return parser_parse_expr_inner(parser);
//ExprOp* parser_parse_expr_inner(parser)
//{
//}

#undef MAKE_OP_FROM_TOKEN
