#include "type.h"

Type* parser_parse_type_list(Parser* parser)
{
    Type  type;
    Type* rhs;
    Token token;

    token = parser_next(parser);
    if (token.type != TOKEN_LEFT_SQUARE)
    {
        fprintf(stderr, "[%s:%d] Type parsing: Logical error, expected '[' token..\n", __FILE__, __LINE__);
        exit(1);
    }

    rhs = parser_parse_type(parser);

    type = (Type)
    {
        .kind = TYPE_LIST     ,
        .identifier = NULL    ,
        .line = token.line    ,
        .column = token.column,
        .length = token.length,
        .type.list = (TypeList)
        {
            .type = rhs,
        }
    };

    return (Type*) arena_push(&parser->arena, &type, sizeof(Type));
}

Type* parser_parse_type_struct(Parser* parser)
{
    Type type;
    int argc = 0;
    TypeStructField** argv;

    Token token;

    token = parser_peek(parser);
    if (token.type != TOKEN_LEFT_BRACE)
    {
        parser_throw_compiler_error(parser, (CompileError)
        {
            .kind   = ERROR_ERROR ,
            .line   = token.line  ,
            .column = token.column,
            .length = token.line  ,
            .msg    = "Type parsing: Expected '{'.",
        });
        return NULL;
    }
    parser_next(parser);

    if (token.type != TOKEN_LEFT_BRACE)
    {
        TypeStructField*  curr_arg = NULL;
        TypeStructField** tmp_ptr = NULL;

        char* identifier = NULL;
        Type* field_type = NULL;
        TypeStructField field;

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

            token = parser_next(parser);
            if (token.type != TOKEN_COLON)
            {
                return NULL;
            }

            field_type = parser_parse_type(parser);
            if (field_type == NULL)
            {
                return NULL;
            }

            field = (TypeStructField)
            {
                .key    = identifier,
                .value  = field_type,
            };

            curr_arg = (TypeStructField*) arena_push(&parser->arena, &field, sizeof(TypeStructField));

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
        argv = (TypeStructField**) arena_push(&parser->arena, tmp_ptr, argc * sizeof(TypeStructField*));
        arrfree(tmp_ptr);
    }
    else
    {
        // No arguments, function is a procedure.
        parser_next(parser);
        argc = 0;
        argv = NULL;
    }

    type = (Type)
    {
        .kind       = TYPE_STRUCT ,
        .identifier = NULL        ,
        .line       = token.line  ,
        .column     = token.column,
        .length     = token.line  ,
        .type.structt = (TypeStruct)
        {
            .argc = argc,
            .argv = argv,
        }
    };

    return (Type*) arena_push(&parser->arena, &type, sizeof(Type));
}

Type* parser_parse_type_function(Parser* parser)
 {
    Token token;
    int argc = 0;
    Type** argv = NULL;
    Type type;

    token = parser_peek(parser);
    if (token.type != TOKEN_LEFT_PAREN)
    {
        parser_throw_compiler_error(parser, (CompileError)
        {
            .kind   = ERROR_ERROR ,
            .line   = token.line  ,
            .column = token.column,
            .length = token.line  ,
            .msg    = "Type parsing: Expected '('.",
        });
        return NULL;
    }
    parser_next(parser);

    token = parser_peek(parser);
    // We check to see if the function is a prcedure or not.
    if (token.type != TOKEN_RIGHT_PAREN)
    {
        Type* curr_arg = NULL;
        Type** tmp_ptr;

        // If it's not, then we parse an argument.
        // Then, we check to see if the token after the parameter is a TOKEN_COMMA or TOKEN_LEFT_PAREN.
        // On TOKEN_COMMA, we continue the loop.
        // On TOKEN_LEFT_PAREN, we exit the loop.
        while (true)
        {
            curr_arg = parser_parse_type(parser);
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
        argv = (Type**) arena_push(&parser->arena, tmp_ptr, argc * sizeof(Type*));
        arrfree(tmp_ptr);
    }
    else
    {
        // No arguments, function is a procedure.
        parser_next(parser);
        argc = 0;
        argv = NULL;
    }

    type = (Type)
    {
        .kind       = TYPE_FN     ,
        .identifier = NULL        ,
        .line       = token.line  ,
        .column     = token.column,
        .length     = token.line  ,
        .type.fn = (TypeFunction)
        {
            .argc = argc,
            .argv = argv,
        }
    };

    return (Type*) arena_push(&parser->arena, &type, sizeof(Type));
}

// Type
Type* parser_parse_type_primitive(Parser* parser)
{
    Type type;

    Token token;

    token = parser_next(parser);
    switch(token.type)
    {
        case TOKEN_NIL_T:
            type.kind = TYPE_NIL ;
            break;
        case TOKEN_BOOL :
            type.kind = TYPE_BOOL;
            break;
        case TOKEN_INT  :
            type.kind = TYPE_INT;
            break;
        case TOKEN_REAL :
            type.kind = TYPE_REAL;
            break;
        case TOKEN_IDENTIFIER:
            type.kind = TYPE_IDENTIFIER;
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

    type = (Type)
    {
        .identifier     = token.start ,
        .line           = token.line  ,
        .column         = token.column,
        .length         = token.length,
        .type.primitive = NULL        , // Assumes primitive, though this should set all pointers to NULL.
    };

    return (Type*) arena_push(&parser->arena, &type, sizeof(Type));
}

Type* parser_parse_type(Parser* parser)
{
    Type* lhs = NULL;
    Type* rhs;

    Token token;

    token = parser_peek(parser);
    switch (token.type)
    {
        case TOKEN_LEFT_BRACE:
            lhs = parser_parse_type_struct(parser);
            break;

        case TOKEN_LEFT_SQUARE:
            lhs = parser_parse_type_list(parser);
            break;

        default:
            // TODO: Currently, we allocate a primitive, and if we manage to parse a 'type function',
            // then we allocate a new function, so we drop the old value and have a new function value.
            // We still allocate a primitive type though, which is means we use memory for no reason.
            // Fix this later.
            lhs = parser_parse_type_primitive(parser);
            token = parser_peek(parser);
            if (token.type == TOKEN_LEFT_PAREN)
            {
                if (lhs->kind == TYPE_IDENTIFIER)
                {
                    rhs = parser_parse_type_function(parser);
                    rhs->identifier = lhs->identifier;
                    rhs->line       = lhs->line      ;
                    rhs->column     = lhs->column    ;
                    rhs->length     = lhs->length    ;
                }
                else
                {
                    parser_throw_compiler_error(parser, (CompileError)
                    {
                        .kind   = ERROR_ERROR ,
                        .line   = token.line  ,
                        .column = token.column,
                        .length = token.line  ,
                        .msg    = "Type parsing: Primitive type is not a type 'function'.",
                    });
                    return NULL;
                }
            }
    }

    return lhs;
}

// Type* construct_type_unknown(Arena* arena)
// {
//     Type type;
// 
//     type = (Type)
//     {
//         .kind           = TYPE_UNKNOWN,
//         .type.primitive = NULL        ,
//     };
//     return (Type*) arena_push(arena, &type, sizeof(Type));
// }
// 
// Type* construct_type_primitive(Arena* arena, TypeKind kind)
// {
//     Type type;
// 
//     if (kind <= TYPE_PRIMITIVE_SEPERATOR || TYPE_DERIVATIVE_SEPERATOR <= kind)
//     {
//         fprintf(stderr, "[%s:%d] Type construction: Cannot construct primitive type by passing a non-primitive TypeKind.\n", __FILE__, __LINE__);
//         exit(1);
//     }
// 
//     type = (Type)
//     {
//         .kind           = kind,
//         .type.primitive = NULL,
//     };
//     return (Type*) arena_push(arena, &type, sizeof(Type));
// }
// 
// Type* construct_type_function(Arena* arena, int argc, Type* argv, Type* return_type)
// {
//     Type type;
// 
//     type = (Type)
//     {
//         .kind    = TYPE_FN,
//         .type.fn = (TypeFunction)
//         {
//             .argc        = argc       ,
//             .argv        = argv       ,
//             .return_type = return_type,
//         };
//     };
// 
//     return arena_push(arena, &type, sizeof(Type));
// }
