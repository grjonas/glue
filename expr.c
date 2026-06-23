#include "expr.h"

// Expr parsing
// Pratt parser
// https://matklad.github.io/2020/04/13/simple-but-powerful-pratt-parsing.html
Expr* parser_parse_expr_inner(Parser* parser, int min_bp) // 'bp' stands for 'binding power'
{
    Expr* lhs = NULL;
    Expr* rhs = NULL;

    Expr        expr_mem       ;
    ExprUnary   expr_unary_mem ;
    ExprBinary  expr_binary_mem;

    // Expr      * expr     = NULL;
    ExprUnary * expr_unary  = NULL;
    ExprBinary* expr_binary = NULL;

    int left_bp = -1, right_bp = -1; // 'bp' stands for 'binding power'

    Token token;

    lhs = parser_parse_expr_primary(parser);
    if (lhs == NULL)
    {
        token = parser_peek(parser);
        expr_unary_mem = get_prefix_operator(token, &right_bp);
        if (expr_unary_mem.kind == EXPR_UNARY_UNKNOWN)
        {
            fprintf(stderr, "[%s:%d] Expression parsing: Unexpected token encountered.\n", __FILE__, __LINE__);
            exit(1);
        }
        parser_next(parser);

        rhs = parser_parse_expr_inner(parser, right_bp);

        expr_unary_mem.unary = rhs;
        expr_unary = (ExprUnary*) arena_push(&parser->arena, &expr_unary_mem, sizeof(ExprUnary));

        expr_mem = (Expr)
        {
            // TODO: Fix txt position information.
            .kind        = EXPR_UNARY  ,
            .line        = token.line  ,
            .column      = token.column,
            .length      = token.length,
            .expr.unary  = expr_unary  ,
        };

        lhs = arena_push(&parser->arena, &expr_mem, sizeof(Expr));
    }

    while (true)
    {
        // Parse binary operator
        token = parser_peek(parser);
        expr_binary_mem = get_infix_operator(token, &left_bp, &right_bp);
        if (expr_binary_mem.kind == EXPR_BINARY_UNKNOWN)
        {
            break;
        }
        if (left_bp < min_bp)
        {
            break;
        }
        parser_next(parser);

        // If managed to parse left hand side, and left_bp < min_bp, we try to parse the right hand side.
        rhs = parser_parse_expr_inner(parser, right_bp);

        expr_binary_mem.left  = lhs;
        expr_binary_mem.right = rhs;
        expr_binary = (ExprBinary*) arena_push(&parser->arena, &expr_binary_mem, sizeof(ExprBinary));

        expr_mem = (Expr)
        {
            // TODO: Fix txt position information.
            .kind        = EXPR_BINARY ,
            .line        = token.line  ,
            .column      = token.column,
            .length      = token.length,
            .expr.binary = expr_binary ,
        };

        lhs = arena_push(&parser->arena, &expr_mem, sizeof(Expr));
    }

    return lhs;
    // fprintf(stderr, "[%s:%d] Expression parsing: Not implemented yet.\n", __FILE__, __LINE__);
    // exit(1);
}

Expr* parser_parse_expr(Parser* parser)
{
    return parser_parse_expr_inner(parser, 0);
}

ExprUnary  get_prefix_operator(Token token, int* right_bp)
{
    ExprUnary expr_unary_mem = (ExprUnary)
    {
        .kind  = EXPR_UNARY_UNKNOWN,
        .unary = NULL              ,
    };

    switch (token.type)
    {
        case TOKEN_NOT : expr_unary_mem.kind = EXPR_UNARY_NOT   ; *right_bp = 11; break;
        case TOKEN_BANG: expr_unary_mem.kind = EXPR_UNARY_NEGATE; *right_bp = 11; break;
        default:
            expr_unary_mem.kind = EXPR_UNARY_UNKNOWN;
            *right_bp = -1;
    }

    return expr_unary_mem;
}

ExprBinary get_infix_operator(Token token, int* left_bp, int* right_bp)
{
    ExprBinary expr_binary_mem = (ExprBinary)
    {
        .kind  = EXPR_BINARY_UNKNOWN,
        .left  = NULL,
        .right = NULL,
    };

    switch (token.type)
    {
        case TOKEN_EQUAL        : expr_binary_mem.kind = EXPR_BINARY_ASSIGN       ; *left_bp =  1; *right_bp =  2; break;
        case TOKEN_OR           : expr_binary_mem.kind = EXPR_BINARY_OR           ; *left_bp =  3; *right_bp =  4; break;
        case TOKEN_AND          : expr_binary_mem.kind = EXPR_BINARY_AND          ; *left_bp =  5; *right_bp =  6; break;
        case TOKEN_EQUAL_EQUAL  : expr_binary_mem.kind = EXPR_BINARY_EQUAL        ; *left_bp =  7; *right_bp =  8; break;
        case TOKEN_BANG_EQUAL   : expr_binary_mem.kind = EXPR_BINARY_NOT_EQUAL    ; *left_bp =  7; *right_bp =  8; break;
        case TOKEN_LESS_EQUAL   : expr_binary_mem.kind = EXPR_BINARY_LESS_EQUAL   ; *left_bp =  9; *right_bp = 10; break;
        case TOKEN_LESS         : expr_binary_mem.kind = EXPR_BINARY_LESS         ; *left_bp =  9; *right_bp = 10; break;
        case TOKEN_GREATER_EQUAL: expr_binary_mem.kind = EXPR_BINARY_GREATER_EQUAL; *left_bp =  9; *right_bp = 10; break;
        case TOKEN_GREATER      : expr_binary_mem.kind = EXPR_BINARY_GREATER      ; *left_bp =  9; *right_bp = 10; break;
        case TOKEN_PLUS         : expr_binary_mem.kind = EXPR_BINARY_ADD          ; *left_bp = 11; *right_bp = 12; break;
        case TOKEN_MINUS        : expr_binary_mem.kind = EXPR_BINARY_SUBTRACT     ; *left_bp = 11; *right_bp = 12; break;
        case TOKEN_STAR         : expr_binary_mem.kind = EXPR_BINARY_MULTIPLY     ; *left_bp = 13; *right_bp = 14; break;
        case TOKEN_SLASH        : expr_binary_mem.kind = EXPR_BINARY_DIVIDE       ; *left_bp = 13; *right_bp = 14; break;
        case TOKEN_PERCENT      : expr_binary_mem.kind = EXPR_BINARY_MODULO       ; *left_bp = 13; *right_bp = 14; break;
        case TOKEN_COLON        : expr_binary_mem.kind = EXPR_BINARY_CHAIN        ; *left_bp = 16; *right_bp = 17; break;
        case TOKEN_DOT          : expr_binary_mem.kind = EXPR_BINARY_ACCESS       ; *left_bp = 16; *right_bp = 17; break;

        default:
            expr_binary_mem.kind = EXPR_BINARY_UNKNOWN;
            *left_bp  = -1;
            *right_bp = -1;
    }

    return expr_binary_mem;
}

// On failure, returns NULL, doesn't change parser state.
// Possible to make the code smaller, but I'm going to refactor this later,
// so I don't want any difficult to anticipate behaviour.
Expr* parser_parse_expr_primary(Parser* parser)
{
    Expr  outer_expr ;
    Expr* expr = NULL;

    ExprPrimary* expr_primary     = NULL;
    ExprPrimary  expr_primary_mem       ;

    Type* type = NULL;
    Type  type_mem   ;

    bool  boolean_value = true;
    char* buffer = NULL;

    Token token = parser_peek(parser);
    switch (token.type)
    {
        case TOKEN_IDENTIFIER:
            parser_next(parser);

            buffer = (char*) arena_push_empty(&parser->arena, (token.length + 1) * sizeof(char));
            memcpy(buffer, token.start, token.length * sizeof(char));

            expr_primary_mem = (ExprPrimary)
            {
                .kind               = EXPR_PRIMARY_IDENTIFIER,
                .primary.identifier = buffer                 ,
            };

            type_mem = (Type)
            {
                .kind           = TYPE_NIL,
                .type.primitive = NULL    ,
            };

            expr_primary = arena_push(&parser->arena, &expr_primary_mem, sizeof(ExprPrimary));
            type         = arena_push(&parser->arena, &type_mem        , sizeof(Type)       );
            break;
        case TOKEN_NIL_V     :
            parser_next(parser);

            expr_primary_mem = (ExprPrimary)
            {
                .kind        = EXPR_PRIMARY_NIL,
                .primary.nil = NULL            ,
            };

            type_mem  = (Type)
            {
                .kind           = TYPE_NIL,
                .type.primitive = NULL    ,
            };

            expr_primary = arena_push(&parser->arena, &expr_primary_mem, sizeof(ExprPrimary));
            type         = arena_push(&parser->arena, &type_mem        , sizeof(Type)       );
            break;

        // TODO: Make the boolean value parsing a little less fragile.
        case TOKEN_FALSE     :
            boolean_value = false;
        case TOKEN_TRUE      :
            parser_next(parser);

            // 'boolean_value = true;' by default
            expr_primary_mem = (ExprPrimary)
            {
                .kind            = EXPR_PRIMARY_BOOLEAN,
                .primary.boolean = boolean_value       ,
            };

            type_mem  = (Type)
            {
                .kind           = TYPE_BOOL   ,
                .type.primitive = NULL        ,
            };

            expr_primary = arena_push(&parser->arena, &expr_primary_mem, sizeof(ExprPrimary));
            type         = arena_push(&parser->arena, &type_mem        , sizeof(Type)       );
            break;

        case TOKEN_INTEGER   :
            parser_next(parser);

            buffer = (char*) arena_push_empty(&parser->arena, (token.length + 1) * sizeof(char));
            memcpy(buffer, token.start, token.length * sizeof(char));

            expr_primary_mem = (ExprPrimary)
            {
                .kind            = EXPR_PRIMARY_INTEGER,
                .primary.integer = buffer              ,
            };

            type_mem  = (Type)
            {
                .kind           = TYPE_INT    ,
                .type.primitive = NULL        ,
            };

            expr_primary = arena_push(&parser->arena, &expr_primary_mem, sizeof(ExprPrimary));
            type         = arena_push(&parser->arena, &type_mem        , sizeof(Type)       );
            break;

        case TOKEN_NUMBER    :
            parser_next(parser);

            buffer = (char*) arena_push_empty(&parser->arena, (token.length + 1) * sizeof(char));
            memcpy(buffer, token.start, token.length * sizeof(char));

            expr_primary_mem = (ExprPrimary)
            {
                .kind         = EXPR_PRIMARY_REAL   ,
                .primary.real = buffer              ,
            };

            type_mem  = (Type)
            {
                .kind           = TYPE_INT    ,
                .type.primitive = NULL        ,
            };

            expr_primary = arena_push(&parser->arena, &expr_primary_mem, sizeof(ExprPrimary));
            type         = arena_push(&parser->arena, &type_mem        , sizeof(Type)       );
            break;

        case TOKEN_STRING    :
            parser_next(parser);

            buffer = (char*) arena_push_empty(&parser->arena, (token.length + 1) * sizeof(char));
            memcpy(buffer, token.start, token.length * sizeof(char));

            expr_primary_mem = (ExprPrimary)
            {
                .kind           = EXPR_PRIMARY_STRING ,
                .primary.string = buffer              ,
            };

            type_mem  = (Type)
            {
                .kind           = TYPE_INT    ,
                .type.primitive = NULL        ,
            };

            expr_primary = arena_push(&parser->arena, &expr_primary_mem, sizeof(ExprPrimary));
            type         = arena_push(&parser->arena, &type_mem        , sizeof(Type)       );
            break;

        case TOKEN_LEFT_PAREN:
            expr = parser_parse_expr_parens(parser);
            if (expr == NULL)
            {
                fprintf(stderr, "[%s:%d] Expression parsing: Could not parse expression inside parentheses.\n", __FILE__, __LINE__);
                exit(1);
            }
            return expr;

        default:
            // fprintf(stderr, "[%s:%d] Expression parsing: Could not parse primary expression.\n", __FILE__, __LINE__);
            // exit(1);
            return NULL;
    }

    outer_expr = (Expr)
    {
        .kind         = EXPR_PRIMARY,
        .type         = type        ,
        .line         = token.line  ,
        .column       = token.column,
        .length       = token.length,
        .expr.primary = expr_primary,
    };

    expr = arena_push(&parser->arena, &outer_expr, sizeof(Expr));
    return expr;
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

// void prefix_binding_power(ExprKind op_kind, int* right)
// {
//     switch (op_kind)
//     {
//         case OP_NEG:
//         case OP_NOT:
//         case OP_PRE_INC:
//         case OP_PRE_DEC:
//             *right = 11;
//             break;
// 
//         default:
//             fprintf(stderr, "Parser line %d: Operator is not prefix.\n", __LINE__);
//             exit(1);
//     }
// }
// 
// bool postfix_binding_power(ExprKind op_kind, int* left)
// {
//     switch (op_kind)
//     {
//         case OP_INDEX:
//         case OP_CALL:
//             *left = 15;
//             break;
// 
//         case OP_POST_INC:
//         case OP_POST_DEC:
//             *left = 13;
//             break;
// 
//         default:
//             return false;
//     }
// 
//     return true;
// }
// 
// bool infix_binding_power(
//     ExprKind op_kind,
//     int* left,
//     int* right)
// {
//     switch (op_kind)
//     {
//         case OP_ASSIGN:
//         case OP_ASSIGN_ADD:
//         case OP_ASSIGN_SUB:
//         case OP_ASSIGN_MUL:
//         case OP_ASSIGN_MOD:
//             *left  = 2;
//             *right = 1;
//             break;
// 
//         case OP_OR:
//             *left  = 3;
//             *right = 4;
//             break;
// 
//         case OP_AND:
//             *left  = 5;
//             *right = 6;
//             break;
// 
//         case OP_EQUAL:
//         case OP_NOT_EQUAL:
//             *left  = 7;
//             *right = 8;
//             break;
// 
//         case OP_GREATER:
//         case OP_LESS:
//         case OP_GREATER_EQUAL:
//         case OP_LESS_EQUAL:
//             *left  = 9;
//             *right = 10;
//             break;
// 
//         case OP_ADD:
//         case OP_SUB:
//             *left  = 11;
//             *right = 12;
//             break;
// 
//         case OP_MUL:
//         case OP_DIV:
//         case OP_MOD:
//             *left  = 13;
//             *right = 14;
//             break;
// 
//         case OP_ACCESS:
//         case OP_CHAIN:
//             *left  = 17;
//             *right = 16;
//             break;
// 
//         default:
//             return false;
//     }
// 
//     return true;
// }
// 
// void append_rhs_to_expr(Expr** expr, Expr** rhs)
// {
//     int rhs_len = arrlen(*rhs);
//     for (int i = 0; i < rhs_len; ++i)
//     {
//         Expr r = *rhs[i];
//         arrput(*expr, r);
//     }
// }
// 
// // TODO: Implement later
// // Expr* parser_parse_expr(Parser* parser)
// // {
// //     return ARENA_NULL;
// // }
// 
// const char* show_op_type(ExprKind op)
// {
//     switch (op)
//     {
//         case OP_STRING: return "OP_STRING";
//         case OP_IDENTIFIER: return "OP_IDENTIFIER";
//         case OP_INTEGER: return "OP_INTEGER";
//         case OP_NUMBER: return "OP_NUMBER";
//         case OP_TRUE: return "OP_TRUE";
//         case OP_FALSE: return "OP_FALSE";
//         case OP_NIL: return "OP_NIL";
//         case OP_PRINT: return "OP_PRINT";
//         case OP_NEG: return "OP_NEG";
//         case OP_NOT: return "OP_NOT";
//         case OP_PRE_INC: return "OP_PRE_INC";
//         case OP_PRE_DEC: return "OP_PRE_DEC";
//         case OP_INDEX: return "OP_INDEX";
//         case OP_POST_INC: return "OP_POST_INC";
//         case OP_POST_DEC: return "OP_POST_DEC";
//         case OP_ADD: return "OP_ADD";
//         case OP_SUB: return "OP_SUB";
//         case OP_MUL: return "OP_MUL";
//         case OP_DIV: return "OP_DIV";
//         case OP_MOD: return "OP_MOD";
//         case OP_AND: return "OP_AND";
//         case OP_OR: return "OP_OR"; 
//         case OP_GREATER: return "OP_GREATER";
//         case OP_LESS: return "OP_LESS";
//         case OP_GREATER_EQUAL: return "OP_GREATER_EQUAL";
//         case OP_LESS_EQUAL: return "OP_LESS_EQUAL";
//         case OP_EQUAL: return "OP_EQUAL";
//         case OP_NOT_EQUAL: return "OP_NOT_EQUAL";
//         case OP_ACCESS: return "OP_ACCESS";
//         case OP_CHAIN: return "OP_CHAIN";
//         case OP_ASSIGN: return "OP_ASSIGN";
//         case OP_ASSIGN_ADD: return "OP_ASSIGN_ADD";
//         case OP_ASSIGN_SUB: return "OP_ASSIGN_SUB";
//         case OP_ASSIGN_MUL: return "OP_ASSIGN_MUL";
//         case OP_ASSIGN_MOD: return "OP_ASSIGN_MOD";
//         case OP_CALL: return "OP_CALL";
//         default:
//             return "UNKNOWN";
//     }
// }

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
