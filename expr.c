#include "expr.h"

// Expr parsing
// Pratt parser
Expr* parser_parse_expr(Parser* parser)
{
//    Token token;
//
//    // First, we parse the primary expressions.
//    token = parser_peek(parser);
//    switch (token)
//    {
//        case TOKEN_IDENTIFIER
//    }

    fprintf(stderr, "[%s:%d] Expression parsing: Not implemented yet.\n", __FILE__, __LINE__);
    exit(1);
}

void prefix_binding_power(ExprKind op_kind, int* right)
{
    switch (op_kind)
    {
        case OP_NEG:
        case OP_NOT:
        case OP_PRE_INC:
        case OP_PRE_DEC:
            *right = 11;
            break;

        default:
            fprintf(stderr, "Parser line %d: Operator is not prefix.\n", __LINE__);
            exit(1);
    }
}

bool postfix_binding_power(ExprKind op_kind, int* left)
{
    switch (op_kind)
    {
        case OP_INDEX:
        case OP_CALL:
            *left = 15;
            break;

        case OP_POST_INC:
        case OP_POST_DEC:
            *left = 13;
            break;

        default:
            return false;
    }

    return true;
}

bool infix_binding_power(
    ExprKind op_kind,
    int* left,
    int* right)
{
    switch (op_kind)
    {
        case OP_ASSIGN:
        case OP_ASSIGN_ADD:
        case OP_ASSIGN_SUB:
        case OP_ASSIGN_MUL:
        case OP_ASSIGN_MOD:
            *left  = 2;
            *right = 1;
            break;

        case OP_OR:
            *left  = 3;
            *right = 4;
            break;

        case OP_AND:
            *left  = 5;
            *right = 6;
            break;

        case OP_EQUAL:
        case OP_NOT_EQUAL:
            *left  = 7;
            *right = 8;
            break;

        case OP_GREATER:
        case OP_LESS:
        case OP_GREATER_EQUAL:
        case OP_LESS_EQUAL:
            *left  = 9;
            *right = 10;
            break;

        case OP_ADD:
        case OP_SUB:
            *left  = 11;
            *right = 12;
            break;

        case OP_MUL:
        case OP_DIV:
        case OP_MOD:
            *left  = 13;
            *right = 14;
            break;

        case OP_ACCESS:
        case OP_CHAIN:
            *left  = 17;
            *right = 16;
            break;

        default:
            return false;
    }

    return true;
}

void append_rhs_to_expr(Expr** expr, Expr** rhs)
{
    int rhs_len = arrlen(*rhs);
    for (int i = 0; i < rhs_len; ++i)
    {
        Expr r = *rhs[i];
        arrput(*expr, r);
    }
}

// TODO: Implement later
// Expr* parser_parse_expr(Parser* parser)
// {
//     return ARENA_NULL;
// }

const char* show_op_type(ExprKind op)
{
    switch (op)
    {
        case OP_STRING: return "OP_STRING";
        case OP_IDENTIFIER: return "OP_IDENTIFIER";
        case OP_INTEGER: return "OP_INTEGER";
        case OP_NUMBER: return "OP_NUMBER";
        case OP_TRUE: return "OP_TRUE";
        case OP_FALSE: return "OP_FALSE";
        case OP_NIL: return "OP_NIL";
        case OP_PRINT: return "OP_PRINT";
        case OP_NEG: return "OP_NEG";
        case OP_NOT: return "OP_NOT";
        case OP_PRE_INC: return "OP_PRE_INC";
        case OP_PRE_DEC: return "OP_PRE_DEC";
        case OP_INDEX: return "OP_INDEX";
        case OP_POST_INC: return "OP_POST_INC";
        case OP_POST_DEC: return "OP_POST_DEC";
        case OP_ADD: return "OP_ADD";
        case OP_SUB: return "OP_SUB";
        case OP_MUL: return "OP_MUL";
        case OP_DIV: return "OP_DIV";
        case OP_MOD: return "OP_MOD";
        case OP_AND: return "OP_AND";
        case OP_OR: return "OP_OR"; 
        case OP_GREATER: return "OP_GREATER";
        case OP_LESS: return "OP_LESS";
        case OP_GREATER_EQUAL: return "OP_GREATER_EQUAL";
        case OP_LESS_EQUAL: return "OP_LESS_EQUAL";
        case OP_EQUAL: return "OP_EQUAL";
        case OP_NOT_EQUAL: return "OP_NOT_EQUAL";
        case OP_ACCESS: return "OP_ACCESS";
        case OP_CHAIN: return "OP_CHAIN";
        case OP_ASSIGN: return "OP_ASSIGN";
        case OP_ASSIGN_ADD: return "OP_ASSIGN_ADD";
        case OP_ASSIGN_SUB: return "OP_ASSIGN_SUB";
        case OP_ASSIGN_MUL: return "OP_ASSIGN_MUL";
        case OP_ASSIGN_MOD: return "OP_ASSIGN_MOD";
        case OP_CALL: return "OP_CALL";
        default:
            return "UNKNOWN";
    }
}

// void print_expr_op(Expr* op)
// {
//     int len = arrlen(op);
//     printf("Expr (len = %d):\n", len);
//     for (int i = 0; i < len; ++i)
//     {
//         Expr e = op[i];
//         // const char* type = show_op_type(e.op_type); 
//         printf(
//             "[%d:%d:%d]: '%.*s'\n",
//             e.line   ,
//             e.column ,
//             e.length ,
//             e.length ,
//             e.literal
//         );
//     }
// }
