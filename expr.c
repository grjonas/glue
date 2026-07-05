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

            // No postfix parsing aside from indexing and function calls
            Expr* outer_expr = NULL;
            token = parser_peek(parser);
            switch (token.type)
            {
                case TOKEN_LEFT_SQUARE:
                    outer_expr = parser_parse_expr_index(parser);

                    if (outer_expr->kind != EXPR_BINARY)
                    {
                        fprintf(stderr, "[%s:%d] Expression parsing: Logical error in pratt parser.\n", __FILE__, __LINE__);
                        exit(1);
                    }
                    // The expression should be pushed already I think.
                    outer_expr->expr.binary.left = lhs;
                    lhs = outer_expr;
                    break;

                case TOKEN_LEFT_PAREN:
                    outer_expr = parser_parse_expr_fn(parser);

                    if (outer_expr->kind != EXPR_FN)
                    {
                        fprintf(stderr, "[%s:%d] Expression parsing: Logical error in pratt parser.\n", __FILE__, __LINE__);
                        exit(1);
                    }
                    // The expression should be pushed already I think.
                    outer_expr->expr.fn.caller = lhs;
                    lhs = outer_expr;
                    break;

                default:
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

            continue;
        }

        if (is_infix(token.type))
        {
            binary_kind = get_infix_operator(token.type, &left_bp, &right_bp);
            if (left_bp < min_bp)
            {
                break;
            }

            parser_next(parser);

            // If managed to parse left hand side, and left_bp < min_bp, we try to parse the right hand side.
            rhs = parser_parse_expr_inner(parser, right_bp);
            if (rhs == NULL)
            {
                parser_throw_compiler_error(parser, (CompileError)
                {
                    .kind   = ERROR_ERROR ,
                    .line   = -1          ,
                    .column = -1          ,
                    .length = -1          ,
                    .msg    = "Expression parsing: Failed to parser right-hand side of expression.",
                });
                return NULL;
            }

            // If it's the chain operator, we create an entirely different expression
            if (binary_kind == EXPR_BINARY_CHAIN)
            {
                // We check if the rhs is a function call - if not, then it's an error
                if (rhs->kind != EXPR_FN)
                {
                    parser_throw_compiler_error(parser, (CompileError)
                    {
                        .kind   = ERROR_ERROR ,
                        .line   = -1          ,
                        .column = -1          ,
                        .length = -1          ,
                        .msg    = "Expression parsing: Cannot chain non-functions.",
                    });
                }
                return NULL;

                // TODO: Fix this hack.
                // Since we do cannot expand the list of function arguments that is already allocated to an arena, we have two choices:
                // 1) We allocate an entirely different list, which is identical to the old one, but has an additional element.
                //     Cons: 
                //     * New memory is allocated, but the old memory sits there idle, which is wasteful.
                //     Pros:
                //     * Easy to implement
                //     
                // 2) Refactor the parser a bit so that we only memory for the list when we know for sure how many elements it has.
                //     Cons:
                //     * Seems difficult to implement, as it would require moving state up the parser from the function call expression, up to the chain operator.
                //     Pros:
                //     * We only use memory we have to.
                //     
                // I went with option 1) for now, but if I ever have to refactor this parser significantly, I should consider option 2).
                // Ease of implementation is the main reason, but we already use significant amounts of memory (which could be optimized later).
                rhs->expr.fn.argc++;

                rhs->expr.fn.argv = create_new_argument_list(&parser->arena, rhs->expr.fn.argc, rhs->expr.fn.argv, lhs);

                lhs = rhs;

                continue;
            }

            expr = (Expr)
            {
                // TODO: Fix txt position information.
                .kind        = EXPR_BINARY ,
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
            .msg    = "Expression parsing: Unexpected token encountered during prefix parsing.",
        });
        return NULL;
    }
    parser_next(parser);

    rhs = parser_parse_expr_inner(parser, right_bp);

    lhs = (Expr)
    {
        .kind   = EXPR_UNARY  ,
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

        case TOKEN_LEFT_BRACE:
            expr_ptr = parser_parse_expr_struct(parser);
            if (expr_ptr == NULL)
            {
                fprintf(stderr, "[%s:%d] Expression parsing: Could not parse struct.\n", __FILE__, __LINE__);
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
        .line         = token.line  ,
        .column       = token.column,
        .length       = token.length,
        .expr.primary = expr_primary,
    };

    expr_ptr = arena_push(&parser->arena, &expr, sizeof(Expr));
    return expr_ptr;
}

Expr* parser_parse_expr_struct(Parser* parser)
{
    Expr expr;

    int argc = 0;
    ExprPrimaryStructField** argv = NULL;

    Token token;

    if (!parser_expect_token(parser, TOKEN_LEFT_BRACE))
        return NULL;

    while (true)
    {
        char    * identifier = NULL;
        TypeExpr* type       = NULL;
        Expr    * value      = NULL;

        ExprPrimaryStructField  field     ;
        ExprPrimaryStructField* arg = NULL;

        identifier = parser_parse_identifier(parser);

        token = parser_peek(parser);
        if (token.type == TOKEN_COLON)
        {
            parser_next(parser);
            type = parser_parse_type_expr(parser);
            if (type == NULL)
            {
                return NULL;
            }
        }

        if (!parser_expect_token(parser, TOKEN_EQUAL))
            return NULL;

        int old_errs = arrlen(parser->errs);
        value = parser_parse_expr_inner(parser, 0);
        if (value == NULL)
        {
            if (arrlen(parser->errs) == old_errs)
            {
                parser_throw_compiler_error(parser, (CompileError)
                {
                    .kind   = ERROR_ERROR ,
                    .line   = token.line  ,
                    .column = token.column,
                    .length = token.line  ,
                    .msg    = "Expression parsing: Failed to parse expression inside of brackets.",
                });
            }
            return NULL;
        }

        field = (ExprPrimaryStructField)
        {
            .key   = identifier,
            .type  = type      ,
            .value = value     ,
        };

        arg = (ExprPrimaryStructField*) arena_push(&parser->arena, &field, sizeof(ExprPrimaryStructField));
        arrput(argv, arg);

        token = parser_next(parser);
        if (token.type == TOKEN_COMMA)
        {
            continue;
        }
        else if (token.type == TOKEN_RIGHT_BRACE)
        {
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
                .msg    = "Expression parsing: Unexpected token encountered.",
            });
            return NULL;
        }
    }

    argc = arrlen(argv);
    ExprPrimaryStructField** tmp_ptr = argv;
    argv = (ExprPrimaryStructField**) arena_push(&parser->arena, argv, argc * sizeof(ExprPrimaryStructField*));
    arrfree(tmp_ptr);

    expr = (Expr)
    {
        .kind   = EXPR_PRIMARY,
        .line   = -1          ,
        .column = -1          ,
        .length = -1          ,
        .expr.primary = (ExprPrimary)
        {
            .kind    = EXPR_PRIMARY_STRUCT,
            .primary.structt = (ExprPrimaryStruct)
            {
                .argc = argc,
                .argv = argv,
            }
        }
    };

    return (Expr*) arena_push(&parser->arena, &expr, sizeof(Expr));
}

Expr* parser_parse_expr_parens(Parser* parser)
{
    if (!parser_expect_token(parser, TOKEN_LEFT_PAREN))
        return NULL;

    int old_errs = arrlen(parser->errs);
    Expr* expr = parser_parse_expr_inner(parser, 0);
    if (expr == NULL)
    {
        if (arrlen(parser->errs) == old_errs)
        {
            parser_throw_compiler_error(parser, (CompileError)
            {
                .kind   = ERROR_ERROR ,
                .line   = -1          ,
                .column = -1          ,
                .length = -1          ,
                .msg    = "Expression parsing: Failed to parse expression inside of parentheses.",
            });
        }
        return NULL;
    }

    if (!parser_expect_token(parser, TOKEN_RIGHT_PAREN))
        return NULL;

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
        case TOKEN_LEFT_SQUARE: *left_bp = 15; return true;
        case TOKEN_LEFT_PAREN : *left_bp = 15; return true;
        default:
            *left_bp = -1;
            return false;
    }
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
