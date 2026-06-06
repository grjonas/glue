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

ExprOp* parser_parse_expr(Parser* parser)
{
    return parser_parse_expr_inner(parser, 0);
}

#define MAKE_OP_FROM_TOKEN(token, oper_type, argsn, expr_type) (ExprOp){\
        .op_type = (oper_type),\
        .type = (expr_type),\
        .args = (argsn),\
        .literal = (token).start,\
        .line = (token).line,\
        .column = (token).column,\
        .length = (token).length\
    } 
// https://matklad.github.io/2020/04/13/simple-but-powerful-pratt-parsing.html
ExprOp* parser_parse_expr_inner(Parser* parser, int8_t min_binding_power)
{
    ExprOp* expr = NULL;
    Token token;

    ExprOp  lhs;
    ExprOp  op ;
    ExprOp* rhs; size_t rhs_len;
    int8_t left_binding_power, right_binding_power;

    // Parse first literal
    token = parser_next(parser);
    LhsOpType lot;
    switch (token.type)
    {
        // Atoms
        case TOKEN_NUMBER    : lhs = MAKE_OP_FROM_TOKEN(token, OP_NUMBER    , 0, TYPE_UNKNOWN); lot = LHS_OP_TYPE_ATOM  ; break;
        case TOKEN_INTEGER   : lhs = MAKE_OP_FROM_TOKEN(token, OP_INTEGER   , 0, TYPE_UNKNOWN); lot = LHS_OP_TYPE_ATOM  ; break;
        case TOKEN_STRING    : lhs = MAKE_OP_FROM_TOKEN(token, OP_STRING    , 0, TYPE_UNKNOWN); lot = LHS_OP_TYPE_ATOM  ; break;
        case TOKEN_IDENTIFIER: lhs = MAKE_OP_FROM_TOKEN(token, OP_IDENTIFIER, 0, TYPE_UNKNOWN); lot = LHS_OP_TYPE_ATOM  ; break;
        // Ops
        case TOKEN_MINUS     : op  = MAKE_OP_FROM_TOKEN(token, OP_NEG    , 1, TYPE_UNKNOWN); lot = LHS_OP_TYPE_PREFIX; break;
        // Special
        case TOKEN_LEFT_PAREN: lot = LHS_OP_TYPE_PARENS; break;
        default:
            fprintf(stderr, "Parser line %d: Could not find atomic expression.\n", __LINE__);
            exit(1);
    }
    switch (lot)
    {
        case LHS_OP_TYPE_ATOM:
            arrput(expr, lhs);
            break;

        case LHS_OP_TYPE_PREFIX:
            prefix_binding_power(op.op_type, &right_binding_power);
            rhs = parser_parse_expr_inner(parser, right_binding_power);

            arrput(expr, op);
            rhs_len = arrlen(rhs);
            for (size_t i = 0; i < rhs_len; ++i)
            {
                arrput(expr, rhs[i]);
            }
            arrfree(rhs);
            break;

        case LHS_OP_TYPE_PARENS:
            token = parser_peek(parser);
            if (token.type == TOKEN_RIGHT_PAREN)
            {
                fprintf(stderr, "Parser line %d: Nothing found in parentheses.\n", __LINE__);
                exit(1);
            }
            rhs = parser_parse_expr_inner(parser, 0);

            rhs_len = arrlen(rhs);
            for (size_t i = 0; i < rhs_len; ++i)
            {
                arrput(expr, rhs[i]);
            }
            arrfree(rhs);
            parser_next(parser);
            break;
        default:
            fprintf(stderr, "Parser line %d: Unknown LhsOpType value.\n", __LINE__);
            exit(1);
    }
    
    while (true)
    {
        bool should_break_loop = false;
        // Parse binary operator
        token = parser_peek(parser);
        switch (token.type)
        {
            case TOKEN_PLUS       : op = MAKE_OP_FROM_TOKEN(token, OP_ADD   , 2, TYPE_UNKNOWN); break;
            case TOKEN_STAR       : op = MAKE_OP_FROM_TOKEN(token, OP_MUL   , 2, TYPE_UNKNOWN); break;
            case TOKEN_LEFT_SQUARE: op = MAKE_OP_FROM_TOKEN(token, OP_INDEX , 2, TYPE_UNKNOWN); break;
            case TOKEN_LEFT_PAREN : op = MAKE_OP_FROM_TOKEN(token, OP_CALL  , 1, TYPE_UNKNOWN); break;
            default:
                should_break_loop = true;
        }
        if (should_break_loop) break;

        // Postfix
        if (postfix_binding_power(op.op_type, &left_binding_power))
        {
            if (left_binding_power < min_binding_power)
                break;
            parser_next(parser); // We probably want to do this now.

            if (op.op_type == OP_INDEX)
            {
                token = parser_peek(parser);
                if (token.type == TOKEN_RIGHT_SQUARE)
                {
                    fprintf(stderr, "Parser line %d: Nothing found in square braces.\n", __LINE__);
                    exit(1);
                }
                rhs = parser_parse_expr_inner(parser, 0);
                parser_next(parser); // Skip second bracket

                arrins(expr, 0, op);
                rhs_len = arrlen(rhs);
                for (size_t i = 0; i < rhs_len; ++i)
                {
                    arrput(expr, rhs[i]);
                }
                arrfree(rhs);
            }
            else if (op.op_type == OP_CALL)
            {
                token = parser_peek(parser);
                if (token.type != TOKEN_RIGHT_PAREN)
                {
                    do
                    {
                        rhs = parser_parse_expr_inner(parser, 0);

                        rhs_len = arrlen(rhs);
                        for (size_t i = 0; i < rhs_len; ++i)
                        {
                            arrput(expr, rhs[i]);
                        }
                        arrfree(rhs);

                        op.args++;

                        token = parser_next(parser); // Skip second bracket

                        if (token.type == TOKEN_RIGHT_PAREN)
                            break;
                        else if (token.type == TOKEN_COMMA)
                            continue;
                        else
                        {
                            fprintf(stderr, "Parser line %d: Function arguments have to be seperated by commas.\n", __LINE__);
                            exit(1);
                        }
                    }
                    while(true);
                }
                arrins(expr, 0, op);
            }
            else
            {
                arrins(expr, 0, op);
                //fprintf(stderr, "Parser line %d: Postfix parsing not implemented.\n", __LINE__);
                //exit(1);
            }

            continue;
        }

        // Infix
        if (infix_binding_power(op.op_type, &left_binding_power, &right_binding_power))
        {
            if (left_binding_power < min_binding_power)
                break;

            parser_next(parser);
            rhs = parser_parse_expr_inner(parser, right_binding_power);
            
            arrins(expr, 0, op);
            rhs_len = arrlen(rhs);
            for (size_t i = 0; i < rhs_len; ++i)
            {
                arrput(expr, rhs[i]);
            }
            arrfree(rhs);
            continue;
        }

        break;
    }

    return expr;
}

void prefix_binding_power(ExprOpType op_type, int8_t* right)
{
    switch (op_type)
    {
        case OP_NEG : *right = 5; break;
        default:
            fprintf(stderr, "Parser line %d: Operator is not prefix.\n", __LINE__);
            exit(1);
    }
}

bool postfix_binding_power(ExprOpType op_type, int8_t* left)
{
    switch (op_type)
    {
        case OP_INDEX : *left = 7; break;
        case OP_CALL  : *left = 7; break;
        default:
            return false;
    }
    return true;
}

bool infix_binding_power(ExprOpType op_type, int8_t* left, int8_t* right)
{
    switch (op_type)
    {
        case OP_ADD : *left = 1; *right = 2; break;
        case OP_MUL : *left = 3; *right = 4; break;
        default:
            return false;
            //fprintf(stderr, "Parser line %d: Operator is not infix.\n", __LINE__);
            //exit(1);
    }
    return true;
}

//    return parser_parse_expr_inner_inner(parser);
//ExprOp* parser_parse_expr_inner_inner(parser)
//{
//}

#undef MAKE_OP_FROM_TOKEN
