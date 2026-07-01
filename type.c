#include "type.h"

// 'bp' stands for binding power
TypeExpr* parser_parse_type_expr_inner(Parser* parser, int min_bp)
{
    TypeExpr type_expr;

    TypeExpr* lhs = NULL;
    TypeExpr* rhs = NULL;

    int left_bp  = 0;
    int right_bp = 0;

    Token token = parser_peek(parser);

    switch (token.type)
    {
        case TOKEN_LEFT_SQUARE:
            lhs = parser_parse_type_expr_list(parser);
            break;

        case TOKEN_LEFT_BRACE:
            lhs = parser_parse_type_expr_struct(parser);
            break;

        default:
            lhs = parser_parse_type_expr_primitive(parser);
            if (lhs == NULL)
            {
                parser_throw_compiler_error(parser, (CompileError)
                {
                    .kind   = ERROR_ERROR ,
                    .line   = token.line  ,
                    .column = token.column,
                    .length = token.line  ,
                    .msg    = "Type parsing: Failed to parser primitive type expression.",
                });
                return NULL;
            }
    }

    do
    {
        token = parser_peek(parser);

        if (token.type == TOKEN_MINUS_GREATER)
        {
            parser_next(parser);

            left_bp  = 1;
            right_bp = 2;

            if (left_bp < min_bp)
            {
                break;
            }

            rhs = parser_parse_type_expr_inner(parser, right_bp);
            if (rhs == NULL)
            {
                return NULL;
            }

            type_expr = (TypeExpr)
            {
                .kind   = TYPE_EXPR_FN,
                .line   = token.line  ,
                .column = token.column,
                .length = token.length,
                .type_expr.fn = (TypeExprFunction)
                {
                    .left  = lhs,
                    .right = rhs,
                }
            };

            lhs = (TypeExpr*) arena_push(&parser->arena, &type_expr, sizeof(TypeExpr));

            continue;
        }
        else if (token.type == TOKEN_LEFT_PAREN)
        {
            if (lhs->kind != TYPE_EXPR_IDENTIFIER)
            {
                parser_throw_compiler_error(parser, (CompileError)
                {
                    .kind   = ERROR_ERROR ,
                    .line   = token.line  ,
                    .column = token.column,
                    .length = token.line  ,
                    .msg    = "Type parsing: Left-hand side of polymorphic type instantiation has to be an identifier.",
                });
                return NULL;
            }

            parser_next(parser);

            left_bp = 10;

            if (left_bp < min_bp)
            {
                break;
            }

            rhs = parser_parse_type_expr_instance(parser);
            assert(rhs->kind == TYPE_EXPR_INSTANCE);

            rhs->type_expr.instance.caller = lhs->type_expr.identifier.identifier;
            lhs = rhs;

            continue;
        }
        else
        {
            break;
        }

    }
    while (true);

    return lhs;
}

TypeExpr* parser_parse_type_expr(Parser* parser)
{
    return parser_parse_type_expr_inner(parser, 0);
}

TypeExpr* parser_parse_type_expr_list(Parser* parser)
{
    TypeExpr  type_expr ;
    TypeExpr* type_inner = NULL;

    Token token;

    token = parser_peek(parser);
    parser_expect_token(parser, TOKEN_LEFT_SQUARE);

    type_inner = parser_parse_type_expr(parser);

    type_expr = (TypeExpr)
    {
        .kind   = TYPE_EXPR_LIST,
        .line   = token.line    ,
        .column = token.column  ,
        .length = token.length  ,
        .type_expr.list = (TypeExprList)
        {
            .type = type_inner
        }
    };

    return (TypeExpr*) arena_push(&parser->arena, &type_expr, sizeof(TypeExpr));
}

TypeExpr* parser_parse_type_expr_struct(Parser* parser)
{
    TypeExpr type_expr;

    int argc = 0;
    TypeExprStructField** argv;

    Token token;

    parser_expect_token(parser, TOKEN_LEFT_BRACE);

    if (token.type != TOKEN_RIGHT_BRACE)
    {
        TypeExprStructField*  curr_arg = NULL;
        TypeExprStructField** tmp_ptr  = NULL;

        char    * identifier = NULL;
        TypeExpr* field_type = NULL;
        TypeExprStructField field;

        // If it's not, then we parse an argument.
        // Then, we check to see if the token after the parameter is a TOKEN_COMMA or TOKEN_LEFT_PAREN.
        // On TOKEN_COMMA, we continue the loop.
        // On TOKEN_LEFT_PAREN, we exit the loop.
        while (true)
        {
            identifier = parser_parse_identifier(parser);
            if (identifier == NULL)
            {
                return NULL;
            }

            parser_expect_token(parser, TOKEN_COLON);

            field_type = parser_parse_type_expr(parser);
            if (field_type == NULL)
            {
                return NULL;
            }

            field = (TypeExprStructField)
            {
                .key    = identifier,
                .value  = field_type,
            };

            curr_arg = (TypeExprStructField*) arena_push(&parser->arena, &field, sizeof(TypeExprStructField));

            arrput(argv, curr_arg);

            token = parser_peek(parser);
            if (token.type == TOKEN_COMMA)
            {
                parser_next(parser);
                continue;
            }
            else if (token.type == TOKEN_RIGHT_BRACE)
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
                    .msg    = "Type parsing: Expected ',' or '}' after function argument.",
                });
                return NULL;
            }
        }

        tmp_ptr = argv;
        argc = arrlen(tmp_ptr);
        argv = (TypeExprStructField**) arena_push(&parser->arena, tmp_ptr, argc * sizeof(TypeExprStructField*));
        arrfree(tmp_ptr);
    }
    else
    {
        // No arguments, function is a procedure.
        parser_next(parser);
        argc = 0;
        argv = NULL;
    }

    type_expr = (TypeExpr)
    {
        .kind       = TYPE_EXPR_STRUCT,
        .line       = token.line      ,
        .column     = token.column    ,
        .length     = token.line      ,
        .type_expr.structt = (TypeExprStruct)
        {
            .argc = argc,
            .argv = argv,
        }
    };

    return (TypeExpr*) arena_push(&parser->arena, &type_expr, sizeof(TypeExpr));
}

TypeExpr* parser_parse_type_expr_instance(Parser* parser)
{
    int argc = 0;
    TypeExpr** argv = NULL;
    TypeExpr   type_expr;

    Token token;

    parser_expect_token(parser, TOKEN_LEFT_PAREN);

    token = parser_peek(parser);
    // We check to see if the function is a prcedure or not.
    if (token.type != TOKEN_RIGHT_PAREN)
    {
        TypeExpr* curr_arg = NULL;
        TypeExpr** tmp_ptr = NULL;

        // If it's not, then we parse an argument.
        // Then, we check to see if the token after the parameter is a TOKEN_COMMA or TOKEN_LEFT_PAREN.
        // On TOKEN_COMMA, we continue the loop.
        // On TOKEN_LEFT_PAREN, we exit the loop.
        while (true)
        {
            curr_arg = parser_parse_type_expr(parser);
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
                    .msg    = "Type parsing: Expected ',' or ')' after function argument.",
                });
                return NULL;
            }
        }

        tmp_ptr = argv;
        argc = arrlen(tmp_ptr);
        argv = (TypeExpr**) arena_push(&parser->arena, tmp_ptr, argc * sizeof(TypeExpr*));
        arrfree(tmp_ptr);
    }
    else
    {
        // No arguments, function is a procedure.
        parser_next(parser);
        argc = 0;
        argv = NULL;
    }

    type_expr = (TypeExpr)
    {
        .kind       = TYPE_EXPR_INSTANCE,
        .line       = token.line        ,
        .column     = token.column      ,
        .length     = token.line        ,
        .type_expr.instance = (TypeExprInstance)
        {
            .caller = NULL,
            .argc   = argc,
            .argv   = argv,
        }
    };

    return (TypeExpr*) arena_push(&parser->arena, &type_expr, sizeof(TypeExpr));
}

// Type
// TODO: Implement parentheses parsing.
TypeExpr* parser_parse_type_expr_primitive(Parser* parser)
{
    TypeExpr type_expr;

    Token token;

    token = parser_peek(parser);
    switch(token.type)
    {
        case TOKEN_NIL_T:
            type_expr.kind = TYPE_EXPR_NIL ;
            break;

        case TOKEN_BOOL :
            type_expr.kind = TYPE_EXPR_BOOL;
            break;

        case TOKEN_INT  :
            type_expr.kind = TYPE_EXPR_INT;
            break;

        case TOKEN_REAL :
            type_expr.kind = TYPE_EXPR_REAL;
            break;

        case TOKEN_STRING:
            type_expr.kind = TYPE_EXPR_STRING;
            break;

        case TOKEN_IDENTIFIER:
            type_expr.kind = TYPE_EXPR_IDENTIFIER;
            break;

        default:
            parser_throw_compiler_error(parser, (CompileError)
            {
                .kind   = ERROR_ERROR ,
                .line   = token.line  ,
                .column = token.column,
                .length = token.line  ,
                .msg    = "Type parsing: Unexpected token encountered.",
            });
            return NULL;
    }
    parser_next(parser);

    type_expr = (TypeExpr)
    {
        .line           = token.line  ,
        .column         = token.column,
        .length         = token.length,
        .type_expr.none = NULL        ,
    };

    return (TypeExpr*) arena_push(&parser->arena, &type_expr, sizeof(TypeExpr));
}

Type* type_convert_type_expr_to_type(Arena* arena, TypeExpr* type_expr)
{
    assert(type_expr != NULL);

    Type type;
    TypeStructField** fields = NULL;

    // Old
    int argc = 0;
    TypeExprStructField** argv= NULL;

    switch (type_expr->kind)
    {
        case TYPE_EXPR_IDENTIFIER:
            type = (Type)
            {
                .kind      = TYPE_VARIABLE,
                .type.none = NULL         ,
            };

            break;

        case TYPE_EXPR_NIL:
            type = (Type)
            {
                .kind      = TYPE_NIL,
                .type.none = NULL    ,
            };
            break;

        case TYPE_EXPR_BOOL:
            type = (Type)
            {
                .kind      = TYPE_BOOL,
                .type.none = NULL     ,
            };
            break;

        case TYPE_EXPR_INT:
            type = (Type)
            {
                .kind      = TYPE_INT ,
                .type.none = NULL     ,
            };
            break;

        case TYPE_EXPR_REAL:
            type = (Type)
            {
                .kind      = TYPE_REAL,
                .type.none = NULL     ,
            };
            break;

        case TYPE_EXPR_STRING:
            type = (Type)
            {
                .kind      = TYPE_STRING,
                .type.none = NULL       ,
            };
            break;

        case TYPE_EXPR_LIST:
            type = (Type)
            {
                .kind = TYPE_LIST,
                .type.list = (TypeList)
                {
                    .type = type_convert_type_expr_to_type(arena, type_expr->type_expr.list.type),
                }
            };
            break;

        case TYPE_EXPR_STRUCT:
            argc = type_expr->type_expr.structt.argc;
            argv = type_expr->type_expr.structt.argv;

            for (int i = 0; i < argc; ++i) 
            {
                TypeExprStructField* f = argv[i];
                TypeStructField converted_field = (TypeStructField)
                {
                    .key   = f->key,
                    .value = type_convert_type_expr_to_type(arena, f->value),
                };
                TypeStructField* new_field = (TypeStructField*) arena_push(arena, &converted_field, sizeof(TypeStructField));
                arrput(fields, new_field);
            }

            type = (Type)
            {
                .kind = TYPE_STRUCT,
                .type.structt = (TypeStruct)
                {
                    .field_num = argc,
                    .fields    = (TypeStructField**) arena_push(arena, fields, argc * sizeof(TypeStructField*)),
                }
            };

            arrfree(fields);
            fields = NULL;
            break;

        case TYPE_EXPR_FN:
            type = (Type)
            {
                .kind = TYPE_FN,
                .type.fn = (TypeFn)
                {
                    .left  = type_convert_type_expr_to_type(arena, type_expr->type_expr.fn.left ),
                    .right = type_convert_type_expr_to_type(arena, type_expr->type_expr.fn.right),
                }
            };
            break;

        case TYPE_EXPR_INSTANCE:
            // struct TypeExprInstance
            // {
                // TypeExpr* caller;
                // int argc;
                // TypeExpr** argv;
            // };

            // type = (Type)
            // {
                // .kind = TYPE_
            // };

            fprintf(stderr, "Type conversion: Not implemented yet.\n");
            exit(1);
            break;

        default:
            assert(false);
    }

    return (Type*) arena_push(arena, &type, sizeof(Type));
}
