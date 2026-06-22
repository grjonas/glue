#include "stmt.h"

// Stmt
// TODO: Implement length calculations when parsing.
Stmt* parser_parse_stmt(Parser* parser)
{
    Token token     ;
    Stmt  outer_stmt;

    Stmt     * stmt       = NULL;
    StmtLet  * stmt_let   = NULL;
    StmtIf   * stmt_if    = NULL;
    StmtWhile* stmt_while = NULL;
    StmtBlock* stmt_block = NULL;
    StmtFn   * stmt_fn    = NULL;
    Expr   * expr       = NULL;

    parser_skip(parser, is_newline);
    token = parser_peek(parser);
    switch (token.type)
    {
        case TOKEN_LET:
            outer_stmt  = (Stmt)
            {
                .kind   = STMT_LET    ,
                .line   = token.line  ,
                .column = token.column,
                .length = -1          ,
            };

            stmt_let = parser_parse_stmt_let(parser);
            if (stmt_let == NULL)
            {
                fprintf(stderr, "[%s:%d] Statement parsing: Failed to parse let statement.\n", __FILE__, __LINE__);
                exit(1);
            }
            outer_stmt.stmt.let = stmt_let;

            stmt = (Stmt*) arena_push(&parser->arena, &outer_stmt, sizeof(Stmt));
            return stmt;

        case TOKEN_IF:
            outer_stmt  = (Stmt)
            {
                .kind   = STMT_IF     ,
                .line   = token.line  ,
                .column = token.column,
                .length = -1          ,
            };

            stmt_if = parser_parse_stmt_if(parser);
            if (stmt_if == NULL)
            {
                fprintf(stderr, "[%s:%d] Statement parsing: Failed to parse if statement.\n", __FILE__, __LINE__);
                exit(1);
            }
            outer_stmt.stmt.if_stmt = stmt_if;

            stmt = (Stmt*) arena_push(&parser->arena, &outer_stmt, sizeof(Stmt));
            return stmt;

        case TOKEN_WHILE:
            outer_stmt  = (Stmt)
            {
                .kind   = STMT_WHILE  ,
                .line   = token.line  ,
                .column = token.column,
                .length = -1          ,
            };

            stmt_while = parser_parse_stmt_while(parser);
            if (stmt_while == NULL)
            {
                fprintf(stderr, "[%s:%d] Statement parsing: Failed to parse while statement.\n", __FILE__, __LINE__);
                exit(1);
            }
            outer_stmt.stmt.while_stmt = stmt_while;

            stmt = (Stmt*) arena_push(&parser->arena, &outer_stmt, sizeof(Stmt));
            return stmt;

        case TOKEN_ERROR:
            return NULL;

        case TOKEN_DO:
            outer_stmt  = (Stmt)
            {
                .kind   = STMT_BLOCK  ,
                .line   = token.line  ,
                .column = token.column,
                .length = -1          ,
            };

            stmt_block = parser_parse_stmt_block(parser);
            if (stmt_block == NULL)
            {
                fprintf(stderr, "[%s:%d] Statement parsing: Failed to parse 'do' block.\n", __FILE__, __LINE__);
                exit(1);
            }
            outer_stmt.stmt.block = stmt_block;

            stmt = (Stmt*) arena_push(&parser->arena, &outer_stmt, sizeof(Stmt));
            return stmt;

        case TOKEN_FN:
            outer_stmt  = (Stmt)
            {
                .kind   = STMT_FN     ,
                .line   = token.line  ,
                .column = token.column,
                .length = -1          ,
            };

            stmt_fn = parser_parse_stmt_fn(parser);
            if (stmt_fn == NULL)
            {
                fprintf(stderr, "[%s:%d] Statement parsing: Failed to parse function.\n", __FILE__, __LINE__);
                exit(1);
            }
            outer_stmt.stmt.fn = stmt_fn;

            stmt = (Stmt*) arena_push(&parser->arena, &outer_stmt, sizeof(Stmt));
            return stmt;
        default:
            // TODO: Implement parsing expression statements.
            outer_stmt  = (Stmt)
            {
                .kind   = STMT_EXPR   ,
                .line   = token.line  ,
                .column = token.column,
                .length = -1          ,
            };

            expr = parser_parse_expr(parser);
            if (expr == NULL)
            {
                fprintf(stderr, "[%s:%d] Statement parsing: Failed to parse expression.\n", __FILE__, __LINE__);
                exit(1);
            }
            outer_stmt.stmt.expr = expr;

            stmt = (Stmt*) arena_push(&parser->arena, &outer_stmt, sizeof(Stmt));
            return stmt;
            // fprintf(stderr, "[%s:%d] Statement parsing: Unexpected token encountered.\n", __FILE__, __LINE__);
            // exit(1);
    }
}

StmtBlock* parser_parse_stmt_block(Parser* parser)
{
    StmtBlock* stmt_block = NULL;
    StmtBlock  block_mem;
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

    block_mem = (StmtBlock)
    {
        .size = 0   ,
        .body = NULL,
    };

    if (parser->current > start + 1)
    {
        Stmt** stmts = NULL;
        Stmt*  stmt  = NULL;

        int   size = 0;

        while ((stmt = parser_parse_stmt(parser)) != NULL)
        {
            arrput(stmts, stmt);
        }

        if (stmts != NULL)
        {
            Stmt** tmp_ptr = NULL;
            size = arrlen(stmts);

            tmp_ptr = (Stmt**) arena_push(&parser->arena, stmts, size * sizeof(Stmt));
            arrfree(stmts);
            stmts = tmp_ptr;
        }

        block_mem = (StmtBlock)
        {
            .size = size ,
            .body = stmts,
        };
    }

    // After return
    parser->start   = start;
    parser->end     = end  ;
    parser->current = start;

    stmt_block = (StmtBlock*) arena_push(&parser->arena, &block_mem, sizeof(StmtBlock));

    return stmt_block;
}

StmtLet* parser_parse_stmt_let(Parser* parser)
{
    StmtLet  stmt_let          ;
    StmtLet* return_stmt = NULL;

    Token    token             ;
    char  *  identifier  = NULL;
    Type  *  type        = NULL;
    Expr*  expr        = NULL;

    stmt_let = (StmtLet)
    {
        .identifier = NULL,
        .type       = NULL,
        .expr       = NULL,
    };

    parser_next(parser);

    identifier = parser_parse_identifier(parser);
    if (identifier == NULL)
    {
        fprintf(stderr, "[%s:%d] Statement parsing: Could not find identifier.\n", __FILE__, __LINE__);
        exit(1);
    }
    stmt_let.identifier = identifier;

    token = parser_peek(parser);
    if (token.type == TOKEN_COLON)
    {
        parser_next(parser);

        type = parser_parse_type(parser);
        if (type == NULL)
        {
            fprintf(stderr, "[%s:%d] Statement parsing: Could not find type.\n", __FILE__, __LINE__);
            exit(1);
        }
        stmt_let.type = type;
    }

    // TODO: Fix this later, not a high priority, but this is kind of bothering me.
    token = parser_peek(parser);
    if (token.type == TOKEN_EQUAL)
    {
        parser_next(parser);

        expr = parser_parse_expr(parser);
        if (expr == NULL)
        {
            fprintf(stderr, "[%s:%d] Statement parsing: Could not find expression.\n", __FILE__, __LINE__);
            exit(1);
        }
        stmt_let.expr = expr;
    }

    return_stmt = (StmtLet*) arena_push(&parser->arena, &stmt_let, sizeof(StmtLet));

    return return_stmt;
}

StmtIf* parser_parse_stmt_if(Parser* parser)
{
    StmtIf  stmt_if;
    StmtIf* return_stmt = NULL;

    Expr* condition   = NULL;
    Stmt  * body        = NULL;

    stmt_if = (StmtIf)
    {
        .condition = NULL,
        .body      = NULL,
        .stmt_else = NULL,
    };

    parser_next(parser);

    condition = parser_parse_expr(parser);
    if (condition == NULL)
    {
        fprintf(stderr, "[%s:%d] Statement parsing: Could not find expression.\n", __FILE__, __LINE__);
        exit(1);
    }
    stmt_if.condition = condition;

    body = parser_parse_stmt(parser);
    if (body == NULL)
    {
        fprintf(stderr, "[%s:%d] Statement parsing: Failed to parse inner statement.\n", __FILE__, __LINE__);
        exit(1);
    }
    stmt_if.body = body;

    // TODO: Implement else branch parsing
    return_stmt = (StmtIf*) arena_push(&parser->arena, &stmt_if, sizeof(StmtIf));

    return return_stmt;
}

StmtWhile* parser_parse_stmt_while(Parser* parser)
{
    StmtWhile  stmt_while        ;
    StmtWhile* return_stmt = NULL;

    Expr   * condition   = NULL;
    Stmt     * body        = NULL;

    stmt_while = (StmtWhile)
    {
        .condition = NULL,
        .body      = NULL,
    };

    parser_next(parser);

    condition = parser_parse_expr(parser);
    if (condition == NULL)
    {
        fprintf(stderr, "[%s:%d] Statement parsing: Could not find expression.\n", __FILE__, __LINE__);
        exit(1);
    }
    stmt_while.condition = condition;

    body = parser_parse_stmt(parser);
    if (body == NULL)
    {
        fprintf(stderr, "[%s:%d] Statement parsing: Failed to parse inner statement.\n", __FILE__, __LINE__);
        exit(1);
    }
    stmt_while.body = body;

    return_stmt = (StmtWhile*) arena_push(&parser->arena, &stmt_while, sizeof(StmtWhile));

    return return_stmt;
}

StmtFn* parser_parse_stmt_fn(Parser* parser)
{
    StmtFn  stmt_fn           ;
    StmtFn* return_stmt = NULL;

    char*       identifier  = NULL;
    int         argc        = 0   ;
    StmtFnArg** argv        = NULL;
    Type*       return_type = NULL;
    Stmt     *  body        = NULL;

    Token token;
    StmtFnArg*  curr_arg = NULL;
    StmtFnArg** tmp_ptr;

    parser_next(parser);

    identifier = parser_parse_identifier(parser);
    if (identifier == NULL)
    {
        fprintf(stderr, "[%s:%d] Statement parsing: Failed to parse identifier.\n", __FILE__, __LINE__);
        exit(1);
    }

    token = parser_next(parser);
    if (token.type != TOKEN_LEFT_PAREN)
    {
        fprintf(stderr, "[%s:%d] Statement parsing: Expected '(' after function name.\n", __FILE__, __LINE__);
        exit(1);
    }

    token = parser_peek(parser);
    // We check to see if the function is a prcedure or not.
    if (token.type != TOKEN_RIGHT_PAREN)
    {
        // If it's not, then we parse an argument.
        // Then, we check to see if the token after the parameter is a TOKEN_COMMA or TOKEN_LEFT_PAREN.
        // On TOKEN_COMMA, we continue the loop.
        // On TOKEN_LEFT_PAREN, we exit the loop.
        while (true)
        {
            curr_arg = parser_parse_stmt_fn_arg(parser);
            if (curr_arg == NULL)
            {
                fprintf(stderr, "[%s:%d] Statement parsing: Failed to parse function argument.\n", __FILE__, __LINE__);
                exit(1);
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
                fprintf(stderr, "[%s:%d] Statement parsing: Expected ',' or ')' after function argument.\n", __FILE__, __LINE__);
                exit(1);
            }
        }

        tmp_ptr = argv;
        argc = arrlen(tmp_ptr);
        argv = (StmtFnArg**) arena_push(&parser->arena, tmp_ptr, argc * sizeof(StmtFnArg));
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
        return_type = parser_parse_type(parser);
        if (return_type == NULL)
        {
            fprintf(stderr, "[%s:%d] Statement parsing: Failed to parse return type.\n", __FILE__, __LINE__);
            exit(1);
        }
    }

    body = parser_parse_stmt(parser);
    if (body == NULL)
    {
        fprintf(stderr, "[%s:%d] Statement parsing: Failed to parse function body.\n", __FILE__, __LINE__);
        exit(1);
    }

    stmt_fn = (StmtFn)
    {
        .identifier  = identifier ,
        .argc        = argc       ,
        .argv        = argv       ,
        .return_type = return_type,
        .body        = body       ,
    };

    return_type = arena_push(&parser->arena, &stmt_fn, sizeof(StmtFn));
    return return_stmt;

    // fprintf(stderr, "[%s:%d] Statement parsing: Function parsing not implemented yet.\n", __FILE__, __LINE__);
    // exit(1);
}

StmtFnArg* parser_parse_stmt_fn_arg(Parser* parser)
{
    StmtFnArg  stmt_fn_arg       ;
    StmtFnArg* return_stmt = NULL;

    Token token;
    char* identifier       = NULL;
    Type* type             = NULL;

    stmt_fn_arg = (StmtFnArg)
    {
        .identifier = NULL,
        .type       = NULL,
    };

    identifier = parser_parse_identifier(parser);
    if (identifier == NULL)
    {
        fprintf(stderr, "[%s:%d] Statement parsing: Failed to parse identifier.\n", __FILE__, __LINE__);
        exit(1);
    }
    stmt_fn_arg.identifier = identifier;

    token = parser_peek(parser);
    if (token.type == TOKEN_COLON)
    {
        parser_next(parser);

        type = parser_parse_type(parser);
        if (type == NULL)
        {
            fprintf(stderr, "[%s:%d] Statement parsing: Failed to parse type.\n", __FILE__, __LINE__);
            exit(1);
        }
        stmt_fn_arg.type = type;
    }

    return_stmt = (StmtFnArg*) arena_push(&parser->arena, &stmt_fn_arg, sizeof(StmtFnArg));

    return return_stmt;
}

// Identifier
char* parser_parse_identifier(Parser* parser)
{
    char* identifier = NULL;
    Token token;

    token = parser_peek(parser);
    if (token.type == TOKEN_IDENTIFIER)
    {
        int   length;
        char* tmp_ptr;

        parser_next(parser);

        length = token.length + 1;
        identifier = calloc(length, sizeof(char));
        if (identifier == NULL)
        {
            fprintf(stderr, "[%s:%d] Failed to allocate memory.\n", __FILE__, __LINE__);
            exit(1);
        }

        // TODO: Check if casting to size_t does what I think it does (it might not because this is C).
        memcpy(identifier, token.start, (size_t) length * sizeof(char));

        tmp_ptr = identifier;
        identifier = (char*) arena_push(&parser->arena, identifier, (size_t) length * sizeof(char));
        free(tmp_ptr);
    }

    return identifier;
}
const char* stmt_type_name(StmtKind kind)
{
    switch (kind)
    {
        case STMT_ERR:               return "STMT_ERR";
        case STMT_LET:               return "STMT_LET";
        case STMT_EXPR:              return "STMT_EXPR";
        case STMT_IF:                return "STMT_IF";
        case STMT_ELIF:              return "STMT_ELIF";
        // case STMT_ELSE:              return "STMT_ELSE";
        case STMT_WHILE:             return "STMT_WHILE";
        case STMT_BREAK:             return "STMT_BREAK";
        case STMT_CONTINUE:          return "STMT_CONTINUE";
        case STMT_FN:                return "STMT_FN";
        case STMT_RETURN:            return "STMT_RETURN";
        case STMT_EMPTY:             return "STMT_EMPTY";
        default:                     return "UNKNOWN";
    }
}
