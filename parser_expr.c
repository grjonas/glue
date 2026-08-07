#include "parser_expr.h"

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
                    TokenType expected[] =
                    {
                        TOKEN_LEFT_SQUARE,
                        TOKEN_LEFT_PAREN
                    };
                    parser_throw_err_unexpected_token(parser, token, expected, 2);
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

            Token err_token = parser_next(parser);

            // If managed to parse left hand side, and left_bp < min_bp, we try to parse the right hand side.
            rhs = parser_parse_expr_inner(parser, right_bp);
            if (rhs == NULL)
            {
                parser_throw_err_generic(parser, err_token, __FILE__, __LINE__);
                return NULL;
            }

            // If it's the chain operator, we create an entirely different expression
            if (binary_kind == EXPR_BINARY_CHAIN)
            {
                // We check if the rhs is a function call - if not, then it's an error
                if (rhs->kind != EXPR_FN)
                {
                    parser_throw_err_generic(parser, err_token, __FILE__, __LINE__);
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
        parser_throw_err_unexpected_prefix_operator(parser, token);
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
            return parser_parse_expr_parens(parser);

        case TOKEN_LEFT_BRACE:
            return parser_parse_expr_struct(parser);

        case TOKEN_LEFT_SQUARE:
            return parser_parse_expr_list(parser);

        case TOKEN_FN         :
            return parser_parse_expr_lambda(parser);

        default:
            fprintf(stderr, "[%s:%d] Expression parsing: Could not parse primary expression.\n", __FILE__, __LINE__);
            exit(1);
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

Expr* parser_parse_expr_list(Parser* parser)
{
    Expr expr;

    int length     = 0   ;
    Expr** list    = NULL;
    Expr*  element = NULL;

    Token token;

    if (!parser_expect_token(parser, TOKEN_LEFT_SQUARE))
        return NULL;

    token = parser_peek(parser);
    if (token.type == TOKEN_RIGHT_SQUARE)
    {
        parser_next(parser);
    }
    else
    {
        while (true)
        {
            element = parser_parse_expr(parser);
            if (element == NULL)
            {
                return NULL;
            }

            arrput(list, element);

            token = parser_peek(parser);
            if (parser_accept_token(parser, TOKEN_RIGHT_SQUARE))
            {
                break;
            }
            else if (parser_accept_token(parser, TOKEN_COMMA))
            {
                continue;
            }
            else
            {
                TokenType expected[] =
                {
                    TOKEN_RIGHT_SQUARE,
                    TOKEN_COMMA,
                };

                parser_throw_err_unexpected_token(parser, token, expected, 2);
                return NULL;
            }
        }
        length = arrlen(list);
        Expr** tmp_ptr = list;
        list = (Expr**) arena_push(&parser->arena, list, length * sizeof(Expr*));
        arrfree(tmp_ptr);
    }

    expr = (Expr)
    {
        .kind   = EXPR_PRIMARY,
        .line   = token.line  ,
        .column = token.column,
        .length = token.length,
        .expr.primary = (ExprPrimary)
        {
            .kind    = EXPR_PRIMARY_LIST,
            .primary.list = (ExprPrimaryList)
            {
                .length = length,
                .list   = list  ,
            }
        }
    };

    return (Expr*) arena_push(&parser->arena, &expr, sizeof(Expr));
}

Expr* parser_parse_expr_struct(Parser* parser)
{
    Expr expr;

    int argc = 0;
    ExprPrimaryStructField** argv = NULL;
    DYNAMIC_ARRAY(char** parsed_keys) = NULL;

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

        Token identifier_token = parser_peek(parser);
        identifier = parser_parse_identifier(parser);
        if (identifier == NULL)
        {
            return NULL;
        }

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

        Token err_token = parser_peek(parser);
        int old_errs = diagnostic_component_get_msg_num(parser->diagnostic_component);
        value = parser_parse_expr_inner(parser, 0);
        if (value == NULL)
        {
            if (diagnostic_component_get_msg_num(parser->diagnostic_component) == old_errs)
            {
                parser_throw_err_generic(parser, err_token, __FILE__, __LINE__);
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

        token = parser_peek(parser);
        if (parser_accept_token(parser, TOKEN_COMMA))
        {
            continue;
        }
        else if (parser_accept_token(parser, TOKEN_RIGHT_BRACE))
        {
            break;
        }
        else
        {
            TokenType expected[] =
            {
                TOKEN_COMMA,
                TOKEN_RIGHT_BRACE,
            };

            parser_throw_err_unexpected_token(parser, token, expected, 2);
            return NULL;
        }

        if (find_string_in_string_list(parsed_keys, identifier) != NULL)
        {
            parser_throw_err_struct_duplicate_identifier(parser, identifier_token);
            return NULL;
        }
        arrput(parsed_keys, identifier);
    }
    arrfree(parsed_keys);

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

Expr* parser_parse_expr_lambda(Parser* parser)
{
    Expr expr;

    int        argc        = 0   ;
    FnArg   ** argv        = NULL;
    TypeExpr * return_type = NULL;
    Stmt     * body        = NULL;

    Token token;

    token = parser_peek(parser);
    parser_expect_token(parser, TOKEN_FN);

    if (!parser_expect_token(parser, TOKEN_LEFT_PAREN))
    {
        return NULL;
    }

    token = parser_peek(parser);
    // We check to see if the function is a prcedure or not.
    if (token.type != TOKEN_RIGHT_PAREN)
    {
        FnArg*  curr_arg = NULL;
        FnArg** tmp_ptr;

        // If it's not, then we parse an argument.
        // Then, we check to see if the token after the parameter is a TOKEN_COMMA or TOKEN_LEFT_PAREN.
        // On TOKEN_COMMA, we continue the loop.
        // On TOKEN_LEFT_PAREN, we exit the loop.
        while (true)
        {
            // TODO: Make is so that arguments cannot have the same identifier as the function name.
            curr_arg = parser_parse_fn_arg(parser);
            if (curr_arg == NULL)
            {
                return NULL;
            }

            arrput(argv, curr_arg);

            token = parser_peek(parser);
            if (parser_accept_token(parser, TOKEN_COMMA))
            {
                continue;
            }
            else if (parser_accept_token(parser, TOKEN_RIGHT_PAREN))
            {
                break;
            }
            else
            {
                TokenType expected[] =
                {
                    TOKEN_COMMA,
                    TOKEN_RIGHT_PAREN,
                };

                parser_throw_err_unexpected_token(parser, token, expected, 2);
                return NULL;
            }
        }

        tmp_ptr = argv;
        argc = arrlen(tmp_ptr);
        argv = (FnArg**) arena_push(&parser->arena, tmp_ptr, argc * sizeof(FnArg*));
        arrfree(tmp_ptr);
    }
    else
    {
        parser_next(parser);
    }

    token = parser_peek(parser);
    if (token.type == TOKEN_COLON)
    {
        parser_next(parser);
        return_type = parser_parse_type_expr(parser);
        if (return_type == NULL)
        {
            return NULL;
        }
    }

    body = parser_parse_stmt(parser);
    if (body == NULL)
    {
        return NULL;
    }

    expr = (Expr)
    {
        .kind = EXPR_PRIMARY,
        .expr.primary = (ExprPrimary)
        {
            .kind = EXPR_PRIMARY_LAMBDA,
            .primary.lambda = (ExprPrimaryLambda)
            {
                .decl        = NULL       ,
                .argc        = argc       ,
                .argv        = argv       ,
                .return_type = return_type,
                .body        = body       ,
            }
        }
    };

    return (Expr*) arena_push(&parser->arena, &expr, sizeof(Expr));
}

Expr* parser_parse_expr_parens(Parser* parser)
{
    Token token = parser_peek(parser);

    if (!parser_expect_token(parser, TOKEN_LEFT_PAREN))
        return NULL;

    int old_errs = diagnostic_component_get_msg_num(parser->diagnostic_component);
    Expr* expr = parser_parse_expr_inner(parser, 0);
    if (expr == NULL)
    {
        if (diagnostic_component_get_msg_num(parser->diagnostic_component) == old_errs)
        {
            parser_throw_err_generic(parser, token, __FILE__, __LINE__);
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

    parser_dont_except_token(parser, TOKEN_RIGHT_SQUARE);

    rhs = parser_parse_expr_inner(parser, 0);

    if (!parser_expect_token(parser, TOKEN_RIGHT_SQUARE))
        return NULL;

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

    if (!parser_expect_token(parser, TOKEN_LEFT_PAREN))
        return NULL;

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
            if (parser_accept_token(parser, TOKEN_COMMA))
            {
                continue;
            }
            else if (parser_accept_token(parser, TOKEN_RIGHT_PAREN))
            {
                break;
            }
            else
            {
                TokenType expected[] =
                {
                    TOKEN_COMMA,
                    TOKEN_RIGHT_PAREN,
                };

                parser_throw_err_unexpected_token(parser, token, expected, 2);
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
