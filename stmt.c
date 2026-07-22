#include "stmt.h"

Stmt* parser_parse_stmts(Parser* parser)
{
    Stmt* stmt_ptr = NULL;
    Stmt  stmt;
    int start  = -1;
    int end    = -1;

    stmt = (Stmt)
    {
        .kind   = STMT_BLOCK  ,
        .line   = 0  ,
        .column = 0,
        .length = strlen(parser->txt),
    };

    start = parser->current;
    end   = parser->end    ;

    stmt.stmt.block = (StmtBlock)
    {
        .size = 0   ,
        .body = NULL,
    };

    if (parser->current >= start)
    {
        Stmt** stmts = NULL;
        int   size = 0;

        while ((stmt_ptr = parser_parse_stmt(parser)) != NULL)
        {
            arrput(stmts, stmt_ptr);
        }

        if (stmts != NULL)
        {
            Stmt** tmp_ptr = NULL;
            size = arrlen(stmts);

            tmp_ptr = (Stmt**) arena_push(&parser->arena, stmts, size * sizeof(Stmt));
            arrfree(stmts);
            stmts = tmp_ptr;
        }

        stmt.stmt.block = (StmtBlock)
        {
            .size = size ,
            .body = stmts,
        };
    }

    // After return
    parser->start   = start;
    parser->end     = end  ;
    parser->current = start;

    stmt_ptr = (Stmt*) arena_push(&parser->arena, &stmt, sizeof(Stmt));
    return stmt_ptr;
}

// Stmt
// TODO: Implement length calculations when parsing.
Stmt* parser_parse_stmt(Parser* parser)
{
    Token token;

    parser_skip(parser, is_newline);
    token = parser_peek(parser);
    switch (token.type)
    {
        case TOKEN_LET:
            return parser_parse_stmt_let(parser);

        case TOKEN_IF:
            return parser_parse_stmt_if(parser, TOKEN_IF);

        case TOKEN_WHILE:
            return parser_parse_stmt_while(parser);

        case TOKEN_ERROR:
            return NULL;

        case TOKEN_DO:
            return  parser_parse_stmt_block(parser);

        case TOKEN_FN:
            return parser_parse_stmt_fn(parser);

        case TOKEN_BREAK:
            return parser_parse_stmt_break(parser);

        case TOKEN_CONTINUE:
            return parser_parse_stmt_continue(parser);

        case TOKEN_RETURN:
            return parser_parse_stmt_return(parser);

        case TOKEN_ALIAS:
            return parser_parse_stmt_alias(parser);

        case TOKEN_TYPE:
            return parser_parse_stmt_type(parser);

        default:
            return parser_parse_stmt_expr(parser);
    }
}

Stmt* parser_parse_stmt_block(Parser* parser)
{
    Stmt* stmt_ptr = NULL;
    Stmt  stmt;
    int start   = -1;
    int end     = -1;
    int current = -1;
    unsigned int depth = 1;

    Token token;

    token = parser_next(parser);
    if (token.type != TOKEN_DO)
    {
        fprintf(stderr, "[%s:%d] Statement parsing: Logical error while parsing statements.\n", __FILE__, __LINE__);
        exit(1);
    }

    stmt = (Stmt)
    {
        .kind   = STMT_BLOCK  ,
        .line   = token.line  ,
        .column = token.column,
        .length = token.length,
    };

    start   = parser->start  ;
    end     = parser->end    ;
    current = parser->current;

    while (depth > 0)
    {
        Token token = parser_next(parser);
        if (token.type == TOKEN_ERROR)
        {
            fprintf(stderr, "[%s:%d] Statement parsing: Could not find the end of 'do' block.\n", __FILE__, __LINE__);
            exit(1);
        }
        else if (token.type == TOKEN_DO)
        {
            ++depth;
        }
        else if (token.type == TOKEN_END)
        {
            --depth;
        }
    }
    parser->start   = current;
    parser->end     = parser->current - 1;
    parser->current = current;

    stmt.stmt.block = (StmtBlock)
    {
        .size = 0   ,
        .body = NULL,
    };

    if (parser->current >= start)
    {
        Stmt** stmts = NULL;
        int   size = 0;

        while ((stmt_ptr = parser_parse_stmt(parser)) != NULL)
        {
            //printf("[%s:%d] Stmt\n", __FILE__, __LINE__);
            arrput(stmts, stmt_ptr);
        }
        //printf("\n");

        if (stmts != NULL)
        {
            Stmt** tmp_ptr = NULL;
            size = arrlen(stmts);

            tmp_ptr = (Stmt**) arena_push(&parser->arena, stmts, size * sizeof(Stmt));
            arrfree(stmts);
            stmts = tmp_ptr;
        }

        stmt.stmt.block = (StmtBlock)
        {
            .size = size ,
            .body = stmts,
        };
    }

    // After return
    parser->start   = start;
    parser->end     = end  ;
    // We assume nothing is between current and 'end' token.
    parser->current += 1;

    stmt_ptr = (Stmt*) arena_push(&parser->arena, &stmt, sizeof(Stmt));
    return stmt_ptr;
}

Stmt* parser_parse_stmt_let(Parser* parser)
{
    Stmt* stmt_ptr = NULL;
    Stmt  stmt;

    char*     identifier = NULL;
    TypeExpr* type   = NULL;
    Expr* expr       = NULL;

    Token token;

    token = parser_next(parser);
    if (token.type != TOKEN_LET)
    {
        fprintf(stderr, "[%s:%d] Statement parsing: Logical error while parsing statements.\n", __FILE__, __LINE__);
        exit(1);
    }

    stmt = (Stmt)
    {
        .kind   = STMT_LET    ,
        .line   = token.line  ,
        .column = token.column,
        .length = token.length,

        .stmt.let = (StmtLet)
        {
            .identifier = NULL,
            .decl       = NULL,
            .type       = NULL,
            .expr       = NULL,
        },
    };

    identifier = parser_parse_identifier(parser);
    if (identifier == NULL)
    {
        parser_throw_compiler_error(parser, (CompileError)
        {
            .kind   = ERROR_ERROR ,
            .line   = token.line  ,
            .column = token.column,
            .length = token.line  ,
            .msg    = "Statement parsing: Could not find identifier.",
        });
        return NULL;
    }
    stmt.stmt.let.identifier = identifier;

    token = parser_peek(parser);
    if (token.type == TOKEN_COLON)
    {
        parser_next(parser);

        type = parser_parse_type_expr(parser);
        if (type == NULL)
        {
            parser_throw_compiler_error(parser, (CompileError)
            {
                .kind   = ERROR_ERROR ,
                .line   = token.line  ,
                .column = token.column,
                .length = token.line  ,
                .msg    = "Statement parsing: Could not find type while parsing the type of let statement.",
            });
            return NULL;
        }
        stmt.stmt.let.type = type;
    }

    // TODO: Fix this later, not a high priority, but this is kind of bothering me.
    token = parser_peek(parser);
    if (token.type == TOKEN_EQUAL)
    {
        parser_next(parser);

        expr = parser_parse_expr(parser);
        if (expr == NULL)
        {
            parser_throw_compiler_error(parser, (CompileError)
            {
                .kind   = ERROR_ERROR ,
                .line   = token.line  ,
                .column = token.column,
                .length = token.line  ,
                .msg    = "Statement parsing: Could not find expression on rhs of let statement.",
            });
            return NULL;
        }
        // This is to make is that the expresseion attached isn't just the expression,
        // but also assigns the result to the variable that is declared.
        expr = construct_assign_expr(&parser->arena, identifier, expr);
        stmt.stmt.let.expr = expr;
    }

    stmt_ptr = (Stmt*) arena_push(&parser->arena, &stmt, sizeof(Stmt));
    return stmt_ptr;
}

Stmt* parser_parse_stmt_if(Parser* parser, TokenType type)
{
    assert(type == TOKEN_IF || type == TOKEN_ELIF || type == TOKEN_ELSE);

    Stmt* stmt_ptr = NULL;
    Stmt  stmt;

    Expr* condition = NULL;
    Stmt* body      = NULL;
    Stmt* next      = NULL;

    Token token;

    token = parser_peek(parser);
    if (!parser_expect_token(parser, type))
        return NULL;

    stmt = (Stmt)
    {
        .kind   = STMT_IF     ,
        .line   = token.line  ,
        .column = token.column,
        .length = token.length,

        .stmt.iff = (StmtIf)
        {
            .condition = NULL,
            .body      = NULL,
            .next      = NULL,
        }
    };

    if (type != TOKEN_ELSE)
    {
        condition = parser_parse_expr(parser);
        if (condition == NULL)
        {
            parser_throw_compiler_error(parser, (CompileError)
            {
                .kind   = ERROR_ERROR ,
                .line   = token.line  ,
                .column = token.column,
                .length = token.line  ,
                .msg    = "Statement parsing: Could not find expression while parsing if statement condition.",
            });
            return NULL;
        }
        stmt.stmt.iff.condition = condition;
    }

    body = parser_parse_stmt(parser);
    if (body == NULL)
    {
        parser_throw_compiler_error(parser, (CompileError)
        {
            .kind   = ERROR_ERROR ,
            .line   = token.line  ,
            .column = token.column,
            .length = token.line  ,
            .msg    = "Statement parsing: Could not find inner statement in if statement.",
        });
        return NULL;
    }
    stmt.stmt.iff.body = body;

    if (type != TOKEN_ELSE)
    {
        parser_skip(parser, is_newline);
        token = parser_peek(parser);
        if (token.type == TOKEN_ELIF)
        {
            next = parser_parse_stmt_if(parser, TOKEN_ELIF);
            if (next == NULL)
            {
                return NULL;
            }
        }
        else if (token.type == TOKEN_ELSE)
        {
            next = parser_parse_stmt_if(parser, TOKEN_ELSE);
            if (next == NULL)
            {
                return NULL;
            }
        }
    }
    stmt.stmt.iff.next = next;


    // TODO: Implement else branch parsing
    stmt_ptr  = (Stmt*) arena_push(&parser->arena, &stmt, sizeof(Stmt));
    return stmt_ptr;
}

Stmt* parser_parse_stmt_while(Parser* parser)
{
    Stmt* stmt_ptr = NULL;
    Stmt  stmt;

    Expr   * condition   = NULL;
    Stmt     * body        = NULL;

    Token token;

    token = parser_next(parser);
    if (token.type != TOKEN_WHILE)
    {
        fprintf(stderr, "[%s:%d] Statement parsing: Logical error while parsing statements.\n", __FILE__, __LINE__);
        exit(1);
    }

    stmt = (Stmt)
    {
        .kind   = STMT_WHILE  ,
        .line   = token.line  ,
        .column = token.column,
        .length = token.length,

        .stmt.whilee = (StmtWhile)
        {
            .condition = NULL,
            .body      = NULL,
        }
    };

    condition = parser_parse_expr(parser);
    if (condition == NULL)
    {
        parser_throw_compiler_error(parser, (CompileError)
        {
            .kind   = ERROR_ERROR ,
            .line   = token.line  ,
            .column = token.column,
            .length = token.line  ,
            .msg    = "Statement parsing: Could not find expression while parsing while statement condition.",
        });
        return NULL;
    }
    stmt.stmt.whilee.condition = condition;

    body = parser_parse_stmt(parser);
    if (body == NULL)
    {
        parser_throw_compiler_error(parser, (CompileError)
        {
            .kind   = ERROR_ERROR ,
            .line   = token.line  ,
            .column = token.column,
            .length = token.line  ,
            .msg    = "Statement parsing: Could not find inner statement in while statement.",
        });
        return NULL;
    }
    stmt.stmt.whilee.body = body;

    stmt_ptr  = (Stmt*) arena_push(&parser->arena, &stmt, sizeof(Stmt));
    return stmt_ptr;
}

Stmt* parser_parse_stmt_break(Parser* parser)
{
    Stmt stmt;
    Token token;

    token = parser_next(parser);
    if (token.type != TOKEN_BREAK)
    {
        fprintf(stderr, "[%s:%d] Statement parsing: Logical error while parsing statements.\n", __FILE__, __LINE__);
        exit(1);
    }

    stmt = (Stmt)
    {
        .kind   = STMT_BREAK  ,
        .line   = token.line  ,
        .column = token.column,
        .length = token.length,

        .stmt.none = NULL     ,
    };

    return (Stmt*) arena_push(&parser->arena, &stmt, sizeof(Stmt));
}


Stmt* parser_parse_stmt_continue(Parser* parser)
{
    Stmt stmt;
    Token token;

    token = parser_next(parser);
    if (token.type != TOKEN_CONTINUE)
    {
        fprintf(stderr, "[%s:%d] Statement parsing: Logical error while parsing statements.\n", __FILE__, __LINE__);
        exit(1);
    }

    stmt = (Stmt)
    {
        .kind   = STMT_CONTINUE,
        .line   = token.line   ,
        .column = token.column ,
        .length = token.length ,

        .stmt.none = NULL      ,
    };

    return (Stmt*) arena_push(&parser->arena, &stmt, sizeof(Stmt));
}

Stmt* parser_parse_stmt_return(Parser* parser)
{
    Stmt stmt;
    Expr* expr = NULL;
    Token token;

    token = parser_next(parser);
    if (token.type != TOKEN_RETURN)
    {
        fprintf(stderr, "[%s:%d] Statement parsing: Logical error while parsing statements.\n", __FILE__, __LINE__);
        exit(1);
    }

    // If it's NULL, then there are no expressions after return.
    int old_errs = arrlen(parser->errs);
    expr = parser_parse_expr(parser);
    if (arrlen(parser->errs) > old_errs)
    {
        return NULL;
    }

    stmt = (Stmt)
    {
        .kind   = STMT_RETURN ,
        .line   = token.line  ,
        .column = token.column,
        .length = token.length,

        .stmt.returnn = (StmtReturn)
        {
            .expr = expr,
            .fn   = NULL,
        }
    };

    return (Stmt*) arena_push(&parser->arena, &stmt, sizeof(Stmt));
}

Stmt* parser_parse_stmt_fn(Parser* parser)
{
    Stmt* stmt_ptr = NULL;
    Stmt  stmt;

    char*       identifier  = NULL;
    int         argc        = 0   ;
    StmtFnArg** argv        = NULL;
    TypeExpr*   return_type = NULL;
    Stmt     *  body        = NULL;

    Token token;

    token = parser_next(parser);
    if (token.type != TOKEN_FN)
    {
        fprintf(stderr, "[%s:%d] Statement parsing: Logical error while parsing statements.\n", __FILE__, __LINE__);
        exit(1);
    }

    stmt = (Stmt)
    {
        .kind   = STMT_FN     ,
        .line   = token.line  ,
        .column = token.column,
        .length = token.length,

        .stmt.fn = (StmtFn)
        {
            .identifier  = NULL,
            .decl        = NULL,
            .argc        = 0   ,
            .argv        = NULL,
            .return_type = NULL,
            .body        = NULL,
        }
    };

    identifier = parser_parse_identifier(parser);
    if (identifier == NULL)
    {
        parser_throw_compiler_error(parser, (CompileError)
        {
            .kind   = ERROR_ERROR ,
            .line   = token.line  ,
            .column = token.column,
            .length = token.line  ,
            .msg    = "Statement parsing: Could not parse function name.",
        });
        return NULL;
    }

    token = parser_next(parser);
    if (token.type != TOKEN_LEFT_PAREN)
    {
        parser_throw_compiler_error(parser, (CompileError)
        {
            .kind   = ERROR_ERROR ,
            .line   = token.line  ,
            .column = token.column,
            .length = token.line  ,
            .msg    = "Statement parsing: Expected '(' after function name.",
        });
        return NULL;
    }

    token = parser_peek(parser);
    // We check to see if the function is a prcedure or not.
    if (token.type != TOKEN_RIGHT_PAREN)
    {
        StmtFnArg*  curr_arg = NULL;
        StmtFnArg** tmp_ptr;

        // If it's not, then we parse an argument.
        // Then, we check to see if the token after the parameter is a TOKEN_COMMA or TOKEN_LEFT_PAREN.
        // On TOKEN_COMMA, we continue the loop.
        // On TOKEN_LEFT_PAREN, we exit the loop.
        while (true)
        {
            // TODO: Make is so that arguments cannot have the same identifier as the function name.
            curr_arg = parser_parse_stmt_fn_arg(parser);
            if (curr_arg == NULL)
            {
                parser_throw_compiler_error(parser, (CompileError)
                {
                    .kind   = ERROR_ERROR ,
                    .line   = token.line  ,
                    .column = token.column,
                    .length = token.line  ,
                    .msg    = "Statement parsing: Failed to parse function argument.",
                });
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
        argv = (StmtFnArg**) arena_push(&parser->arena, tmp_ptr, argc * sizeof(StmtFnArg*));
        arrfree(tmp_ptr);
    }
    else
    {
        parser_next(parser);
    }

//    int old_errs = arrlen(parser->errs);
//    if (arrlen(parser->errs) > old_errs)
//    {
//        parser_throw_compiler_error(parser, (CompileError)
//        {
//            .kind   = ERROR_ERROR ,
//            .line   = token.line  ,
//            .column = token.column,
//            .length = token.line  ,
//            .msg    = "Statement parsing: Failed to parse function return type.",
//        });
//        return NULL;
//    }

    token = parser_peek(parser);
    if (token.type == TOKEN_COLON)
    {
        parser_next(parser);
        return_type = parser_parse_type_expr(parser);
        if (return_type == NULL)
        {
            parser_throw_compiler_error(parser, (CompileError)
            {
                .kind   = ERROR_ERROR ,
                .line   = token.line  ,
                .column = token.column,
                .length = token.line  ,
                .msg    = "Statement parsing: Failed to parse function return type.",
            });
            return NULL;
        }
    }
    // This is not something that should be done,
    // since there's not telling the return type of a function
    // if it's type expression is ommited.
    // else
    // {
    //     return_type = construct_primitive_type_expr(&parser->arena, TYPE_EXPR_NIL);
    // }

    body = parser_parse_stmt(parser);
    if (body == NULL)
    {
        parser_throw_compiler_error(parser, (CompileError)
        {
            .kind   = ERROR_ERROR ,
            .line   = token.line  ,
            .column = token.column,
            .length = token.line  ,
            .msg    = "Statement parsing: Failed to parse function body.",
        });
        return NULL;
    }

    stmt.stmt.fn = (StmtFn)
    {
        .identifier  = identifier ,
        .decl        = NULL       ,
        .argc        = argc       ,
        .argv        = argv       ,
        .return_type = return_type,
        .body        = body       ,
    };

    stmt_ptr = (Stmt*) arena_push(&parser->arena, &stmt, sizeof(Stmt));
    return stmt_ptr;
}

StmtFnArg* parser_parse_stmt_fn_arg(Parser* parser)
{
    StmtFnArg  stmt_fn_arg       ;
    StmtFnArg* return_stmt = NULL;

    Token token;
    char* identifier       = NULL;
    TypeExpr* type             = NULL;

    stmt_fn_arg = (StmtFnArg)
    {
        .identifier = NULL,
        .decl       = NULL,
        .type       = NULL,
    };

    identifier = parser_parse_identifier(parser);
    if (identifier == NULL)
    {
        parser_throw_compiler_error(parser, (CompileError)
        {
            .kind   = ERROR_ERROR ,
            .line   = token.line  ,
            .column = token.column,
            .length = token.line  ,
            .msg    = "Statement parsing: Failed to parse function argument name.",
        });
        return NULL;
    }
    stmt_fn_arg.identifier = identifier;

    token = parser_peek(parser);
    if (token.type == TOKEN_COLON)
    {
        parser_next(parser);

        type = parser_parse_type_expr(parser);
        if (type == NULL)
        {
            parser_throw_compiler_error(parser, (CompileError)
            {
                .kind   = ERROR_ERROR ,
                .line   = token.line  ,
                .column = token.column,
                .length = token.line  ,
                .msg    = "Statement parsing: Failed to parse function argument type declaration.",
            });
            return NULL;
        }
        stmt_fn_arg.type = type;
    }

    return_stmt = (StmtFnArg*) arena_push(&parser->arena, &stmt_fn_arg, sizeof(StmtFnArg));
    return return_stmt;
}

Stmt* parser_parse_stmt_expr(Parser* parser)
{
    Stmt* stmt_ptr = NULL;
    Stmt stmt;
    Expr* expr = NULL;

    stmt.kind = STMT_EXPR;
    stmt.stmt.expr = NULL     ;

    Token token = parser_peek(parser);
    expr = parser_parse_expr(parser);
    if (expr == NULL)
    {
        parser_throw_compiler_error(parser, (CompileError)
        {
            .kind   = ERROR_ERROR ,
            .line   = token.line  ,
            .column = token.column,
            .length = token.line  ,
            .msg    = "Statement parsing: Failed to parse expression.",
        });
        return NULL;
    }
    stmt.line   = expr->line  ;
    stmt.column = expr->column;
    stmt.length = expr->length;
    stmt.stmt.expr = expr;

    stmt_ptr = (Stmt*) arena_push(&parser->arena, &stmt, sizeof(Stmt));
    return stmt_ptr;
}

Stmt* parser_parse_stmt_alias(Parser* parser)
{
    Stmt stmt;
    char* identifier = NULL;
    TypeExpr* type = NULL;
    Token token;

    // Parse keyword
    token = parser_peek(parser);
    parser_expect_token(parser, TOKEN_ALIAS);

    // Parse identifier
    identifier = parser_parse_identifier(parser);
    if (identifier== NULL)
    {
        parser_throw_compiler_error(parser, (CompileError)
        {
            .kind   = ERROR_ERROR ,
            .line   = token.line  ,
            .column = token.column,
            .length = token.line  ,
            .msg    = "Statement parsing: Failed to parse identifier in 'alias' statement.",
        });
        return NULL;
    }

    // Parse type
    type = parser_parse_type_expr(parser);
    if (type == NULL)
    {
        parser_throw_compiler_error(parser, (CompileError)
        {
            .kind   = ERROR_ERROR ,
            .line   = token.line  ,
            .column = token.column,
            .length = token.line  ,
            .msg    = "Statement parsing: Failed to parse type in 'alias' statement.",
        });
        return NULL;
    }

    stmt = (Stmt)
    {
        .kind   = STMT_ALIAS  ,
        .line   = token.line  ,
        .column = token.column,
        .length = token.length,
        .stmt.alias = (StmtAlias)
        {
            .identifier = identifier,
            .type       = type      ,
            .decl       = NULL      ,
        }
    };

    return (Stmt*) arena_push(&parser->arena, &stmt, sizeof(Stmt));
}

StmtTypeConstructor* parser_parse_stmt_type_constructor(Parser* parser)
{
    StmtTypeConstructor constructor;

    char*      identifier = NULL;
    int        type_num   = 0   ;
    TypeExpr** types      = NULL;

    TypeExpr*  type       = NULL;

    Token token;

    if (!parser_expect_token(parser, TOKEN_PIPE))
        return NULL;

    identifier = parser_parse_identifier(parser);
    if (identifier == NULL)
    {
        return NULL;
    }

    if (!parser_expect_token(parser, TOKEN_LEFT_PAREN))
        return NULL;

    if (token.type != TOKEN_RIGHT_PAREN)
    {
        do
        {
            type = parser_parse_type_expr(parser);
            if (type == NULL)
            {
                return NULL;
            }
            arrput(types, type);

            token = parser_next(parser);
            if (token.type == TOKEN_RIGHT_PAREN)
            {
                break;
            }
            else if (token.type == TOKEN_COMMA)
            {
                continue;
            }
            else
            {
                parser_throw_compiler_error(parser, (CompileError)
                {
                    .kind   = ERROR_ERROR ,
                    .line   = token.line  ,
                    .column = token.column,
                    .length = token.line  ,
                    .msg    = "Statement parsing: Unexpected token encountered",
                });
                return NULL;
            }
        }
        while (true);
    }

    type_num = arrlen(types);
    TypeExpr** tmp_ptr = types;
    types = (TypeExpr**) arena_push(&parser->arena, types, type_num * sizeof(TypeExpr*));
    arrfree(tmp_ptr);

    constructor = (StmtTypeConstructor)
    {
        .identifier = identifier,
        .type_num   = type_num  ,
        .types      = types     ,
    };

    return (StmtTypeConstructor*) arena_push(&parser->arena, &constructor, sizeof(StmtTypeConstructor));
}

Stmt* parser_parse_stmt_type(Parser* parser)
{
    Stmt stmt;

    char               *  identifier      = NULL;
    int                   argc            = 0   ;
    int                   constructor_num = 0   ;
    char               ** argv            = NULL;
    StmtTypeConstructor** constructors    = NULL;

    char               *  arg             = NULL;
    StmtTypeConstructor*  constructor     = NULL;

    if (!parser_expect_token(parser, TOKEN_TYPE))
        return NULL;

    identifier = parser_parse_identifier(parser);
    if (identifier == NULL)
    {
        return NULL;
    }

    if (!parser_expect_token(parser, TOKEN_LEFT_PAREN))
        return NULL;

    // parse argument list
    Token token = parser_peek(parser);
    if (token.type != TOKEN_RIGHT_PAREN)
    {
        do
        {
            arg = parser_parse_identifier(parser);
            if (arg == NULL)
            {
                return NULL;
            }
            arrput(argv, arg);

            token = parser_next(parser);
            if (token.type == TOKEN_RIGHT_PAREN)
            {
                break;
            }
            else if (token.type == TOKEN_COMMA)
            {
                continue;
            }
            else
            {
                parser_throw_compiler_error(parser, (CompileError)
                {
                    .kind   = ERROR_ERROR ,
                    .line   = token.line  ,
                    .column = token.column,
                    .length = token.line  ,
                    .msg    = "Statement parsing: Unexpected token encountered",
                });
                return NULL;
            }
        }
        while (true);
    }

    argc = arrlen(argv);
    char** tmp_ids = argv;
    argv = (char**) arena_push(&parser->arena, argv, argc * sizeof(char*));
    arrfree(tmp_ids);

    // parse contructors
    do
    {
        parser_skip(parser, is_newline);

        token = parser_peek(parser);

        if (token.type == TOKEN_PIPE)
        {
            constructor = parser_parse_stmt_type_constructor(parser);
            if (constructor == NULL)
            {
                return NULL;
            }
            arrput(constructors, constructor);
        }
        else if (token.type == TOKEN_END)
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
                .msg    = "Statement parsing: Failed to type constructor in 'type' statement.",
            });
            return NULL;
        }

    }
    while (true);

    constructor_num = arrlen(constructors);
    StmtTypeConstructor** tmp_ptr  = constructors;
    constructors = (StmtTypeConstructor**) arena_push(&parser->arena, constructors, constructor_num * sizeof(StmtTypeConstructor*));
    arrfree(tmp_ptr);

    stmt = (Stmt)
    {
        .kind = STMT_TYPE,
        .line   = -1,
        .column = -1,
        .length = -1,
        .stmt.type = (StmtType)
        {
            .identifier      = identifier     ,
            .argv            = argv           ,
            .constructors    = constructors   ,
            .argc            = argc           ,
            .constructor_num = constructor_num,
            .decl            = NULL           ,
        }
    };

    return (Stmt*) arena_push(&parser->arena, &stmt, sizeof(Stmt));
}
