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

#define MAKE_OP_FROM_TOKEN(token, oper_type, expr_type) (ExprOp){\
        .op_type = (oper_type),\
        .type = (expr_type),\
        .literal = (token).start,\
        .line = (token).line,\
        .column = (token).column,\
        .length = (token).length\
    } 
ExprOp* parser_parse_expr(Parser* parser, int8_t min_binding_power)
{
    ExprOp* expr = NULL;
    Token token;

    ExprOp  lhs;
    ExprOp  op ;
    ExprOp* rhs;
    int8_t left_binding_power, right_binding_power;

    // Parse first literal
    token = parser_next(parser);
    switch (token.type)
    {
        case TOKEN_NUMBER : lhs = MAKE_OP_FROM_TOKEN(token, OP_NUMBER , TYPE_UNKNOWN); break;
        case TOKEN_INTEGER: lhs = MAKE_OP_FROM_TOKEN(token, OP_INTEGER, TYPE_UNKNOWN); break;
        //case TOKEN_IDENTIFIER: expr = MAKE_OP_FROM_TOKEN(token, OP_IDENTIFIER, 0, TYPE_UNKNOWN); break;
        default:
            fprintf(stderr, "Parser line %d: Could not find atomic expression.\n", __LINE__);
            exit(1);
    }
    arrput(expr, lhs);
    
    while (true)
    {
        bool should_break_loop = false;
        // Parse binary operator
        token = parser_peek(parser);
        switch (token.type)
        {
            case TOKEN_PLUS : op = MAKE_OP_FROM_TOKEN(token, OP_ADD, TYPE_UNKNOWN); break;
            case TOKEN_STAR : op = MAKE_OP_FROM_TOKEN(token, OP_MUL, TYPE_UNKNOWN); break;
            default:
                should_break_loop = true;
        }
        if (should_break_loop) break;

        infix_binding_power(op.op_type, &left_binding_power, &right_binding_power);
        if (left_binding_power < min_binding_power)
            break;

        parser_next(parser);
        rhs = parser_parse_expr(parser, right_binding_power);
        
        arrins(expr, 0, op);
        size_t rhs_len = arrlen(rhs);
        for (size_t i = 0; i < rhs_len; ++i)
        {
            arrput(expr, rhs[i]);
        }
        arrfree(rhs);
    }

    return expr;
}

void infix_binding_power(ExprOpType op_type, int8_t* left, int8_t* right)
{
    switch (op_type)
    {
        case OP_ADD : *left = 1; *right = 2; break;
        case OP_MUL : *left = 3; *right = 4; break;
        default:
            fprintf(stderr, "Parser line %d: Operator is not infix.\n", __LINE__);
            exit(1);
    }
}

//    return parser_parse_expr_inner(parser);
//ExprOp* parser_parse_expr_inner(parser)
//{
//}

#undef MAKE_OP_FROM_TOKEN
