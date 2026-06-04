#include "parser.h"

Parser init_parser(Scanner scanner)
{
    Parser parser =
    {
        .txt = scanner.init,
        .tokens = scanner.token_list,
        .info =
        {
            .start    = 0,
            .end      = arrlen(scanner.token_list),
            .current  = 0
        }
    };

    return parser;
}

ExprOp* parser_parse_expr(Parser parser)
{
    return parser_parse_expr_inner(parser, 0);
}

#define MAKE_OP_FROM_TOKEN(expr_op, token, oper_type, args_num, expr_type) (expr_op) = (ExprOp){\
        .op_type = (oper_type),\
        .args = (args_num),\
        .type = (expr_type),\
        .literal = (token).start,\
        .literal_size = (token).length,\
        .line = (token).line,\
        .column = (token).column,\
        .length = (token).length\
    } 
ExprOp* parser_parse_expr_inner(Parser parser, int8_t minimum_binding_power)
{
    ExprOp* expr_ops = NULL;
    Token token;
    ExprOp lhs ;
    ExprOp op  ;
    ExprOp* rhs = NULL;
    int32_t rhs_len;
    int8_t left_binding_power, right_binding_power;

    // Left-hand side
    token = parser.tokens[parser.info.current++];
    switch (token.type)
    {
        // Atoms
        case TOKEN_TRUE      : MAKE_OP_FROM_TOKEN(lhs, token, OP_TRUE      , 0, EXPR_TYPE_UNKNOWN); break;
        case TOKEN_FALSE     : MAKE_OP_FROM_TOKEN(lhs, token, OP_FALSE     , 0, EXPR_TYPE_UNKNOWN); break;
        case TOKEN_NIL       : MAKE_OP_FROM_TOKEN(lhs, token, OP_NIL       , 0, EXPR_TYPE_UNKNOWN); break;
        case TOKEN_INTEGER   : MAKE_OP_FROM_TOKEN(lhs, token, OP_INTEGER   , 0, EXPR_TYPE_UNKNOWN); break;
        case TOKEN_NUMBER    : MAKE_OP_FROM_TOKEN(lhs, token, OP_NUMBER    , 0, EXPR_TYPE_UNKNOWN); break;
        case TOKEN_STRING    : MAKE_OP_FROM_TOKEN(lhs, token, OP_STRING    , 0, EXPR_TYPE_UNKNOWN); break;
        case TOKEN_IDENTIFIER: MAKE_OP_FROM_TOKEN(lhs, token, OP_IDENTIFIER, 0, EXPR_TYPE_UNKNOWN); break;
        // Prefix operators
        case TOKEN_MINUS:
            MAKE_OP_FROM_TOKEN(op, token, OP_NEG, 1, EXPR_TYPE_UNKNOWN);
            prefix_binding_power(op, &right_binding_power);
            rhs = parser_parse_expr_inner(parser, right_binding_power);

            arrput(expr_ops, op);
            rhs_len = arrlen(rhs);
            for (int32_t i = 0; i < rhs_len; ++i)
                arrput(expr_ops, rhs[i]);
            arrfree(rhs);
            break;
        case TOKEN_NOT:
            MAKE_OP_FROM_TOKEN(op, token, OP_NOT, 1, EXPR_TYPE_UNKNOWN);
            prefix_binding_power(op, &right_binding_power);
            rhs = parser_parse_expr_inner(parser, right_binding_power);

            arrput(expr_ops, op);
            rhs_len = arrlen(rhs);
            for (int32_t i = 0; i < rhs_len; ++i)
                arrput(expr_ops, rhs[i]);
            arrfree(rhs);
            break;
        default:
            fprintf(stderr, "Bad token in line %d: '%.*s'\n", __LINE__, token.length, token.start);
            exit(1);
    }

    while (true)
    {
        token = parser.tokens[parser.info.current];
        switch (token.type)
        {
            case TOKEN_AND          : MAKE_OP_FROM_TOKEN(op, token, OP_AND          , 2, EXPR_TYPE_UNKNOWN); break;
            case TOKEN_OR           : MAKE_OP_FROM_TOKEN(op, token, OP_OR           , 2, EXPR_TYPE_UNKNOWN); break;
            case TOKEN_PLUS         : MAKE_OP_FROM_TOKEN(op, token, OP_ADD          , 2, EXPR_TYPE_UNKNOWN); break;
            case TOKEN_MINUS        : MAKE_OP_FROM_TOKEN(op, token, OP_SUB          , 2, EXPR_TYPE_UNKNOWN); break;
            case TOKEN_STAR         : MAKE_OP_FROM_TOKEN(op, token, OP_MUL          , 2, EXPR_TYPE_UNKNOWN); break;
            case TOKEN_SLASH        : MAKE_OP_FROM_TOKEN(op, token, OP_DIV          , 2, EXPR_TYPE_UNKNOWN); break;
            case TOKEN_PERCENT      : MAKE_OP_FROM_TOKEN(op, token, OP_MOD          , 2, EXPR_TYPE_UNKNOWN); break;
            case TOKEN_GREATER      : MAKE_OP_FROM_TOKEN(op, token, OP_GREATER      , 2, EXPR_TYPE_UNKNOWN); break;
            case TOKEN_LESS         : MAKE_OP_FROM_TOKEN(op, token, OP_LESS         , 2, EXPR_TYPE_UNKNOWN); break;
            case TOKEN_GREATER_EQUAL: MAKE_OP_FROM_TOKEN(op, token, OP_GREATER_EQUAL, 2, EXPR_TYPE_UNKNOWN); break;
            case TOKEN_LESS_EQUAL   : MAKE_OP_FROM_TOKEN(op, token, OP_LESS_EQUAL   , 2, EXPR_TYPE_UNKNOWN); break;
            case TOKEN_EQUAL_EQUAL  : MAKE_OP_FROM_TOKEN(op, token, OP_EQUAL        , 2, EXPR_TYPE_UNKNOWN); break;
            case TOKEN_SLASH_EQUAL  : MAKE_OP_FROM_TOKEN(op, token, OP_NOT_EQUAL    , 2, EXPR_TYPE_UNKNOWN); break;
            case TOKEN_DOT          : MAKE_OP_FROM_TOKEN(op, token, OP_ACCESS       , 2, EXPR_TYPE_UNKNOWN); break;
            //TOKEN_COLON, // technically binary, but it has different behaviour
            default:
                fprintf(stderr, "Bad token in line %d: '%.*s'\n", __LINE__, token.length, token.start);
                exit(1);
        }

        infix_binding_power(op.op_type, &left_binding_power, &right_binding_power);
        if (left_binding_power < minimum_binding_power)
            break;

        parser.info.current++;
        rhs = parser_parse_expr_inner(parser, right_binding_power);

        arrput(expr_ops, op);
        arrput(expr_ops, lhs);
        rhs_len = arrlen(rhs);
        for (int32_t i = 0; i < rhs_len; ++i)
            arrput(expr_ops, rhs[i]);
        arrfree(rhs);
    }

    //if (expr_ops == NULL)
    //{
    //    fprintf(stderr, "Something has gone seriously wrong in line %d: %s.\n", __LINE__, parser.txt);
    //    exit(1);
    //}
    return expr_ops;
}
#undef MAKE_OP_FROM_TOKEN

//1 + 2 == 3 and 1 * 2 * 5 + 1 /= 0 or true

void infix_binding_power(ExprOpType op, int8_t* left, int8_t* right)
{
    switch (op)
    {
        case OP_AND          : *left = 1; *right = 2; break;
        case OP_OR           : *left = 1; *right = 2; break;
        case OP_ADD          : *left = 1; *right = 2; break;
        case OP_SUB          : *left = 1; *right = 2; break;
        case OP_MUL          : *left = 2; *right = 3; break;
        case OP_DIV          : *left = 2; *right = 3; break;
        case OP_MOD          : *left = 2; *right = 3; break;
        case OP_GREATER      : *left = 1; *right = 2; break;
        case OP_LESS         : *left = 1; *right = 2; break;
        case OP_GREATER_EQUAL: *left = 1; *right = 2; break;
        case OP_LESS_EQUAL   : *left = 1; *right = 2; break;
        case OP_EQUAL        : *left = 1; *right = 2; break;
        case OP_NOT_EQUAL    : *left = 1; *right = 2; break;
        case OP_ACCESS       : *left = 6; *right = 5; break;
        default:
            fprintf(stderr, "Bad op in line %d.\n", __LINE__);
    }
}

void prefix_binding_power(ExprOp op, int8_t* right)
{
    switch (op.op_type)
    {
        case OP_NEG  : *right = 5; break;
        case OP_NOT  : *right = 5; break;
        default:
            fprintf(stderr, "Bad op in line %d.\n", __LINE__);
    }
}
