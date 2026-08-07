#include "expr.h"

ExprUnaryKind get_prefix_operator(TokenType type, int* right_bp)
{
    ExprUnaryKind kind = EXPR_UNARY_UNKNOWN;

    switch (type)
    {
        case TOKEN_NOT : kind = EXPR_UNARY_NOT   ; *right_bp = 11; break;
        case TOKEN_BANG: kind = EXPR_UNARY_NEGATE; *right_bp = 11; break;
        default:
            kind = EXPR_UNARY_UNKNOWN;
            *right_bp = -1;
    }

    return kind;
}

ExprBinaryKind get_infix_operator(TokenType type, int* left_bp, int* right_bp)
{
    ExprBinaryKind kind = EXPR_BINARY_UNKNOWN;;

    switch (type)
    {
        case TOKEN_EQUAL        : kind = EXPR_BINARY_ASSIGN       ; *left_bp =  1; *right_bp =  2; break;
        case TOKEN_OR           : kind = EXPR_BINARY_OR           ; *left_bp =  3; *right_bp =  4; break;
        case TOKEN_AND          : kind = EXPR_BINARY_AND          ; *left_bp =  5; *right_bp =  6; break;
        case TOKEN_EQUAL_EQUAL  : kind = EXPR_BINARY_EQUAL        ; *left_bp =  7; *right_bp =  8; break;
        case TOKEN_BANG_EQUAL   : kind = EXPR_BINARY_NOT_EQUAL    ; *left_bp =  7; *right_bp =  8; break;
        case TOKEN_LESS_EQUAL   : kind = EXPR_BINARY_LESS_EQUAL   ; *left_bp =  9; *right_bp = 10; break;
        case TOKEN_LESS         : kind = EXPR_BINARY_LESS         ; *left_bp =  9; *right_bp = 10; break;
        case TOKEN_GREATER_EQUAL: kind = EXPR_BINARY_GREATER_EQUAL; *left_bp =  9; *right_bp = 10; break;
        case TOKEN_GREATER      : kind = EXPR_BINARY_GREATER      ; *left_bp =  9; *right_bp = 10; break;
        case TOKEN_PLUS         : kind = EXPR_BINARY_ADD          ; *left_bp = 11; *right_bp = 12; break;
        case TOKEN_MINUS        : kind = EXPR_BINARY_SUBTRACT     ; *left_bp = 11; *right_bp = 12; break;
        case TOKEN_STAR         : kind = EXPR_BINARY_MULTIPLY     ; *left_bp = 13; *right_bp = 14; break;
        case TOKEN_SLASH        : kind = EXPR_BINARY_DIVIDE       ; *left_bp = 13; *right_bp = 14; break;
        case TOKEN_PERCENT      : kind = EXPR_BINARY_MODULO       ; *left_bp = 13; *right_bp = 14; break;
        case TOKEN_COLON        : kind = EXPR_BINARY_CHAIN        ; *left_bp = 16; *right_bp = 17; break;
        case TOKEN_DOT          : kind = EXPR_BINARY_ACCESS       ; *left_bp = 16; *right_bp = 17; break;

        default:
            kind = EXPR_BINARY_UNKNOWN;
            *left_bp  = -1;
            *right_bp = -1;
    }

    return kind;
}

ExprUnaryKind get_postfix_operator(TokenType type, int* right_bp)
{
    ExprUnaryKind kind = EXPR_UNARY_UNKNOWN;

    switch (type)
    {
        default:
            kind = EXPR_UNARY_UNKNOWN;
            *right_bp = -1;
    }

    return kind;
}

bool is_infix(TokenType type)
{
    int left_bp  = -1;
    int right_bp = -1;

    return get_infix_operator(type, &left_bp, &right_bp) == EXPR_BINARY_UNKNOWN ? false : true;
}

bool is_postfix(TokenType type, int* left_bp)
{
    switch (type)
    {
        case TOKEN_LEFT_SQUARE: *left_bp = 15; return true;
        case TOKEN_LEFT_PAREN : *left_bp = 15; return true;
        default:
            *left_bp = -1;
            return false;
    }
}

Expr* construct_assign_expr(Arena* arena, char* identifier, Expr* expr)
{
    assert(identifier != NULL);
    assert(expr       != NULL);

    Expr identifier_expr = (Expr)
    {
        .kind = EXPR_PRIMARY,
        .expr.primary = (ExprPrimary)
        {
            .kind = EXPR_PRIMARY_IDENTIFIER,
            .primary.identifier = identifier,
        }
    };

    Expr assign_expr = (Expr)
    {
        .kind = EXPR_BINARY,
        .expr.binary = (ExprBinary)
        {
            .kind  = EXPR_BINARY_ASSIGN,
            .left  = (Expr*) arena_push(arena, &identifier_expr, sizeof(Expr)),
            .right = expr,
        }
    };

    return (Expr*) arena_push(arena, &assign_expr, sizeof(Expr));
}

Expr** create_new_argument_list(Arena* arena, int old_argc, Expr** expr, Expr* lhs)
{
    // rhs->expr.fn.argv = create_new_argument_list(&parser->arena, rhs->expr.fn.argv, lhs);

    Expr** new_expr = NULL;

    new_expr = (Expr**) arena_push_empty(arena, (old_argc + 1) * sizeof(Expr*));

    for (int i = 0; i < old_argc; ++i)
    {
        new_expr[i] = expr[i];
    }
    new_expr[old_argc] = lhs;

    return new_expr;
}
