#include "expr.h"

// Expr parsing
// Pratt parser
// https://matklad.github.io/2020/04/13/simple-but-powerful-pratt-parsing.html
Expr* parser_parse_expr_inner(Parser* parser, int min_bp) // 'bp' stands for 'binding power'
{
    Expr* lhs = NULL;
    Expr* rhs = NULL;

    Expr expr;
    ExprUnaryKind  unary_kind ;
    ExprBinaryKind binary_kind;

    int left_bp = -1, right_bp = -1; // 'bp' stands for 'binding power'

    Token token;

    lhs = parser_parse_expr_primary(parser);
    if (lhs == NULL)
    {
        lhs = parser_parse_expr_prefix(parser);
        if (lhs == NULL)
        {
            return NULL;
        }
    }

    // This is SO not clean, but I don't know if this code can be made clean to be honest.
    while (true)
    {
        // Parse binary operator
        token = parser_peek(parser);
        if (is_postfix(token.type, &left_bp))
        {
            if (left_bp < min_bp)
            {
                break;
            }
            // parser_next(parser); /* We probably do not want to do this now */

            Expr* outer_expr = NULL;
            outer_expr = parser_parse_expr_index(parser);
            if (outer_expr == NULL)
            {
                outer_expr = parser_parse_expr_fn(parser);
                if (outer_expr == NULL)
                {
                    parser_throw_compiler_error(parser, (CompileError)
                    {
                        .kind   = ERROR_ERROR ,
                        .line   = token.line  ,
                        .column = token.column,
                        .length = token.line  ,
                        .msg    = "Expression parsing: Unexpected token encountered.",
                    });
                    return NULL;
                    // Not implemented
                    // outer_expr = parser_parse_expr_unary_postfix(parser);
                    // if (outer_expr == NULL)
                    // {
                    //     fprintf(stderr, "[%s:%d] Expression parsing: Logical error in pratt parser.\n", __FILE__, __LINE__);
                    //     exit(1);
                    // }
                    // else
                    // {
                    //     if (outer.expr.kind != EXPR_UNARY)
                    //     {
                    //         fprintf(stderr, "[%s:%d] Expression parsing: Logical error in pratt parser.\n", __FILE__, __LINE__);
                    //         exit(1);
                    //     }
                    //     // The expression should be pushed already I think.
                    //     outer_expr->expr.unary.unary = lhs;
                    //     lhs = outer_expr;
                    // }
                }
                else
                {
                    if (outer_expr->kind != EXPR_FN)
                    {
                        fprintf(stderr, "[%s:%d] Expression parsing: Logical error in pratt parser.\n", __FILE__, __LINE__);
                        exit(1);
                    }
                    // The expression should be pushed already I think.
                    outer_expr->expr.fn.caller = lhs;
                    lhs = outer_expr;
                }
            }
            else
            {
                if (outer_expr->kind != EXPR_BINARY)
                {
                    fprintf(stderr, "[%s:%d] Expression parsing: Logical error in pratt parser.\n", __FILE__, __LINE__);
                    exit(1);
                }
                // The expression should be pushed already I think.
                outer_expr->expr.binary.left = lhs;
                lhs = outer_expr;
            }

            continue;
        }

        if (is_infix(token.type))
        {
            binary_kind = get_infix_operator(token.type, &left_bp, &right_bp);
            if (left_bp < right_bp)
            {
                break;
            }

            parser_next(parser);

            // If managed to parse left hand side, and left_bp < min_bp, we try to parse the right hand side.
            rhs = parser_parse_expr_inner(parser, right_bp);

            expr = (Expr)
            {
                // TODO: Fix txt position information.
                .kind        = EXPR_BINARY ,
                .type        = NULL        ,
                .line        = token.line  ,
                .column      = token.column,
                .length      = token.length,
                .expr.binary = (ExprBinary)
                {
                    .kind  = binary_kind,
                    .left  = lhs        ,
                    .right = rhs        ,
                }
            };

            lhs = arena_push(&parser->arena, &expr, sizeof(Expr));

            continue;
        }

        break;
    }

    return lhs;
}

Expr* parser_parse_expr(Parser* parser)
{
    return parser_parse_expr_inner(parser, 0);
}

Expr* parser_parse_expr_prefix(Parser* parser)
{
    Expr  lhs;
    Expr* rhs;
    ExprUnaryKind unary_kind;
    int right_bp = -1;

    Token token;

    token = parser_peek(parser);
    unary_kind  = get_prefix_operator(token.type, &right_bp);
    if (unary_kind == EXPR_UNARY_UNKNOWN)
    {
        parser_throw_compiler_error(parser, (CompileError)
        {
            .kind   = ERROR_ERROR ,
            .line   = token.line  ,
            .column = token.column,
            .length = token.line  ,
            .msg    = "Expression parsing: Unexpected token encountered.",
        });
        return NULL;
    }
    parser_next(parser);

    rhs = parser_parse_expr_inner(parser, right_bp);

    lhs = (Expr)
    {
        .kind   = EXPR_UNARY  ,
        .type   = NULL        ,
        .line   = token.line  ,
        .column = token.column,
        .length = token.length,

        .expr.unary = (ExprUnary)
        {
            .kind  = unary_kind,
            .unary = rhs       ,
        }
    };

    return arena_push(&parser->arena, &lhs, sizeof(Expr));
}

// On failure, returns NULL, doesn't change parser state.
// Possible to make the code smaller, but I'm going to refactor this later,
// so I don't want any difficult to anticipate behaviour.
Expr* parser_parse_expr_primary(Parser* parser)
{
    Expr* expr_ptr = NULL;
    Expr  expr;
    ExprPrimary expr_primary;

    bool  boolean_value = true;
    char* buffer = NULL;

    Token token = parser_peek(parser);
    switch (token.type)
    {
        case TOKEN_IDENTIFIER:
            parser_next(parser);

            buffer = (char*) arena_push_empty(&parser->arena, (token.length + 1) * sizeof(char));
            memcpy(buffer, token.start, token.length * sizeof(char));

            expr_primary = (ExprPrimary)
            {
                .kind               = EXPR_PRIMARY_IDENTIFIER,
                .primary.identifier = buffer                 ,
            };

            break;

        case TOKEN_NIL_V     :
            parser_next(parser);

            expr_primary = (ExprPrimary)
            {
                .kind        = EXPR_PRIMARY_NIL,
                .primary.nil = NULL            ,
            };

            break;

        // TODO: Make the boolean value parsing a little less fragile.
        case TOKEN_FALSE     :
            boolean_value = false;
        case TOKEN_TRUE      :
            parser_next(parser);

            // 'boolean_value = true;' by default
            expr_primary = (ExprPrimary)
            {
                .kind            = EXPR_PRIMARY_BOOLEAN,
                .primary.boolean = boolean_value       ,
            };

            break;

        case TOKEN_INTEGER   :
            parser_next(parser);

            buffer = (char*) arena_push_empty(&parser->arena, (token.length + 1) * sizeof(char));
            memcpy(buffer, token.start, token.length * sizeof(char));

            expr_primary = (ExprPrimary)
            {
                .kind            = EXPR_PRIMARY_INTEGER,
                .primary.integer = buffer              ,
            };

            break;

        case TOKEN_NUMBER    :
            parser_next(parser);

            buffer = (char*) arena_push_empty(&parser->arena, (token.length + 1) * sizeof(char));
            memcpy(buffer, token.start, token.length * sizeof(char));

            expr_primary = (ExprPrimary)
            {
                .kind         = EXPR_PRIMARY_REAL   ,
                .primary.real = buffer              ,
            };
            break;

        case TOKEN_STRING    :
            parser_next(parser);

            buffer = (char*) arena_push_empty(&parser->arena, (token.length + 1) * sizeof(char));
            memcpy(buffer, token.start, token.length * sizeof(char));

            expr_primary = (ExprPrimary)
            {
                .kind           = EXPR_PRIMARY_STRING ,
                .primary.string = buffer              ,
            };

            break;

        case TOKEN_LEFT_PAREN:
            expr_ptr = parser_parse_expr_parens(parser);
            if (expr_ptr == NULL)
            {
                fprintf(stderr, "[%s:%d] Expression parsing: Could not parse expression inside parentheses.\n", __FILE__, __LINE__);
                exit(1);
            }
            return expr_ptr;

        default:
            // fprintf(stderr, "[%s:%d] Expression parsing: Could not parse primary expression.\n", __FILE__, __LINE__);
            // exit(1);
            return NULL;
    }

    expr = (Expr)
    {
        .kind         = EXPR_PRIMARY,
        .type         = NULL        ,
        .line         = token.line  ,
        .column       = token.column,
        .length       = token.length,
        .expr.primary = expr_primary,
    };

    expr_ptr = arena_push(&parser->arena, &expr, sizeof(Expr));
    return expr_ptr;
}

// Maybe this is a bit overengineered,
// but in the event I refactor the parser and forget to change this,
// This should come in handy.
Expr* parser_parse_expr_parens(Parser* parser)
{
    Expr* expr = NULL;
    int start  = -1;
    int end    = -1;

    unsigned int depth = 1;

    parser_next(parser);
    start = parser->current;
    end   = parser->end    ;

    while (depth > 0)
    {
        Token token = parser_next(parser);
        if (token.type == TOKEN_ERROR)
        {
            fprintf(stderr, "[%s:%d] Expression parsing: Could not find matching parenthese.\n", __FILE__, __LINE__);
            exit(1);
        }
        else if (token.type == TOKEN_LEFT_PAREN)
        {
            ++depth;
        }
        else if (token.type == TOKEN_RIGHT_PAREN)
        {
            --depth;
        }
    }

    expr = parser_parse_expr(parser);

    // After return
    parser->start   = start;
    parser->end     = end  ;
    parser->current = start;

    return expr;
}

Expr* parser_parse_expr_index(Parser* parser)
{
    Expr  expr;
    Expr* rhs      = NULL;

    // We check if we have the right token.
    Token token = parser_next(parser);
    if (token.type != TOKEN_LEFT_SQUARE)
    {
        fprintf(stderr, "[%s:%d] Expression parsing: Logical error in pratt parser.\n", __FILE__, __LINE__);
        exit(1);
    }

    token = parser_peek(parser);
    if (token.type == TOKEN_RIGHT_SQUARE)
    {
        parser_throw_compiler_error(parser, (CompileError)
        {
            .kind   = ERROR_ERROR ,
            .line   = token.line  ,
            .column = token.column,
            .length = token.line  ,
            .msg    = "Expression parsing: Nothing found in square brackets.",
        });
        return NULL;
    }

    rhs = parser_parse_expr_inner(parser, 0);

    token = parser_next(parser);
    if (token.type != TOKEN_RIGHT_SQUARE)
    {
        parser_throw_compiler_error(parser, (CompileError)
        {
            .kind   = ERROR_ERROR ,
            .line   = token.line  ,
            .column = token.column,
            .length = token.line  ,
            .msg    = "Expression parsing: Exprected right square bracket immediately after index argument.",
        });
        return NULL;
    }

    expr = (Expr)
    {
        // TODO: Fix txt position information.
        .kind        = EXPR_BINARY ,
        .type        = NULL        ,
        .line        = token.line  ,
        .column      = token.column,
        .length      = token.length,
        .expr.binary = (ExprBinary)
        {
            .kind  = EXPR_BINARY_INDEX,
            .left  = NULL             ,
            .right = rhs,
        }
    };

    return (Expr*) arena_push(&parser->arena, &expr, sizeof(Expr));
}

Expr* parser_parse_expr_fn(Parser* parser)
{
    Token token;
    int argc = 0;
    Expr** argv = NULL;
    Expr expr;

    token = parser_peek(parser);
    if (token.type != TOKEN_LEFT_PAREN)
    {
        parser_throw_compiler_error(parser, (CompileError)
        {
            .kind   = ERROR_ERROR ,
            .line   = token.line  ,
            .column = token.column,
            .length = token.line  ,
            .msg    = "Expression parsing: Expected '('.",
        });
        return NULL;
    }
    parser_next(parser);

    token = parser_peek(parser);
    // We check to see if the function is a prcedure or not.
    if (token.type != TOKEN_RIGHT_PAREN)
    {
        Expr* curr_arg = NULL;
        Expr** tmp_ptr;

        // If it's not, then we parse an argument.
        // Then, we check to see if the token after the parameter is a TOKEN_COMMA or TOKEN_LEFT_PAREN.
        // On TOKEN_COMMA, we continue the loop.
        // On TOKEN_LEFT_PAREN, we exit the loop.
        while (true)
        {
            curr_arg = parser_parse_expr_inner(parser, 0);
            if (curr_arg == NULL)
            {
                return NULL;
            }

            arrput(argv, curr_arg);
            token = parser_peek(parser);
            if (token.type == TOKEN_COMMA)
            {
                parser_next(parser);
                continue;
            }
            else if (token.type == TOKEN_RIGHT_PAREN)
            {
                parser_next(parser);
                break;
            }
            else
            {
                parser_throw_compiler_error(parser, (CompileError)
                {
                    .kind   = ERROR_ERROR ,
                    .line   = token.line  ,
                    .column = token.column,
                    .length = token.line  ,
                    .msg    = "Statement parsing: Expected ',' or ')' after function argument.",
                });
                return NULL;
            }
        }

        tmp_ptr = argv;
        argc = arrlen(tmp_ptr);
        argv = (Expr**) arena_push(&parser->arena, tmp_ptr, argc * sizeof(Expr*));
        arrfree(tmp_ptr);
    }
    else
    {
        // No arguments, function is a procedure.
        parser_next(parser);
        argc = 0;
        argv = NULL;
    }

    expr = (Expr)
    {
        // TODO: Fix txt position information.
        .kind        = EXPR_FN     ,
        .type        = NULL        ,
        .line        = token.line  ,
        .column      = token.column,
        .length      = token.length,
        .expr.fn = (ExprFn)
        {
            .argc   = argc,
            .argv   = argv,
            .caller = NULL,
        }
    };

    return (Expr*) arena_push(&parser->arena, &expr, sizeof(Expr));
}

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
        case TOKEN_RIGHT_SQUARE: *left_bp = 15; return true;
        case TOKEN_RIGHT_PAREN : *left_bp = 15; return true;
        default:
            *left_bp = -1;
            return false;
    }
}
