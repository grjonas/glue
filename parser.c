#include "parser.h"

#define arrfree_and_set_null(op) do { arrfree(op); op = NULL; } while(0)

Parser init_parser(Scanner scanner)
{
    Parser parser =
    {
        .state        = PARSER_STATE_UNPARSED     ,
        .txt          = scanner.init              ,
        .tokens       = scanner.token_list        ,
        .start        = 0                         ,
        .end          = arrlen(scanner.token_list),
        .current      = 0                         ,
        .log          = NULL                      ,
    };

    return parser;
}

Token parser_peek(Parser* parser)
{
    const char* err = "No more tokens left.";
    if (parser->current >= parser->end)
    {
        return (Token)
        {
            .type   = TOKEN_ERROR,
            .start  = err,
            .line   = -1,
            .column = -1,
            .length = strlen(err)
        };
    }
    return parser->tokens[parser->current];
}

Token parser_next(Parser* parser)
{
    Token token = parser_peek(parser);
    if (token.type != TOKEN_ERROR)
        parser->current++;
    return token;
}


bool parser_skip(Parser* parser, bool (*predicate)(TokenType))
{
    bool ret = false;
    while (true)
    {
        Token token = parser_peek(parser);
        if (predicate(token.type))
        {
            parser_next(parser);
            ret = true;
        }
        else
            break;
    }
    return ret;
}

bool is_newline(TokenType type)
{
    switch (type)
    {
        case TOKEN_SEMICOLON: return true;
        case TOKEN_NEWLINE  : return true;
        default: return false;
    }
}

// Stmt
Stmt* parser_parse_stmt(Parser* parser)
{
    Token token;

    Stmt    stmt      ;
    StmtLet stmt_let  ;

    char  * identifier;
    Type  * type      ;
    ExprOp* expr      ;

    parser_skip(parser, is_newline);
    token = parser_peek(parser);
    switch (token.type)
    {
        case TOKEN_LET:
            parser_next(parser);
            stmt_let = (StmtLet)
            {
                .identifier = NULL,
                .type       = NULL,
                .expr       = NULL,
            };

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

            fprintf(stderr, "[%s:%d] Statement parsing: Not implemented yet.\n", __FILE__, __LINE__);
            exit(1);
        default:
            fprintf(stderr, "[%s:%d] Statement parsing: Unexpected token encountered.\n", __FILE__, __LINE__);
            exit(1);
    }
}

// Identifier
char* parser_parse_identifier(Parser* parser)
{
    fprintf(stderr, "[%s:%d] Identifier parsing is not implemented yet.\n", __FILE__, __LINE__);
    exit(1);
}

// Type
Type* parser_parse_type(Parser* parser)
{
    return parser_parse_type_inner(parser);
}

Type* parser_parse_type_inner(Parser* parser)
{
    fprintf(stderr, "[%s:%d] Type parsing is not implemented yet.\n", __FILE__, __LINE__);
    exit(1);
}

ExprOp* parser_parse_expr(Parser* parser)
{
    ExprOp* expr  = parser_parse_expr_inner(parser, 0);
    return expr;
}

// TODO: Rework this to add literals to parser storage.
// https://matklad.github.io/2020/04/13/simple-but-powerful-pratt-parsing.html
#define MAKE_OP_FROM_TOKEN(token, oper_type, argsn, expr_type) (ExprOp){\
        .op_type = (oper_type),\
        .type = (expr_type),\
        .args = (argsn),\
        .literal = (token).start,\
        .line = (token).line,\
        .column = (token).column,\
        .length = (token).length\
    } 
ExprOp* parser_parse_expr_inner(Parser* parser, int min_binding_power)
{
    ExprOp* expr = NULL;
    Token token;

    ExprOp  lhs;
    ExprOp  op ;
    ExprOp* rhs = NULL;
    int left_binding_power, right_binding_power;

    // Parse first literal
    token = parser_next(parser);
    LhsOpType lot;
    switch (token.type)
    {
        // Atoms
        case TOKEN_NIL       : lhs = MAKE_OP_FROM_TOKEN(token, OP_NIL       , 0, TYPE_NIL    ); lot = LHS_OP_TYPE_ATOM; break;
        case TOKEN_TRUE      : lhs = MAKE_OP_FROM_TOKEN(token, OP_TRUE      , 0, TYPE_BOOL   ); lot = LHS_OP_TYPE_ATOM; break;
        case TOKEN_FALSE     : lhs = MAKE_OP_FROM_TOKEN(token, OP_FALSE     , 0, TYPE_BOOL   ); lot = LHS_OP_TYPE_ATOM; break;
        case TOKEN_NUMBER    : lhs = MAKE_OP_FROM_TOKEN(token, OP_NUMBER    , 0, TYPE_FLOAT  ); lot = LHS_OP_TYPE_ATOM  ; break;
        case TOKEN_INTEGER   : lhs = MAKE_OP_FROM_TOKEN(token, OP_INTEGER   , 0, TYPE_INT    ); lot = LHS_OP_TYPE_ATOM  ; break;
        case TOKEN_STRING    : lhs = MAKE_OP_FROM_TOKEN(token, OP_STRING    , 0, TYPE_UNKNOWN); lot = LHS_OP_TYPE_ATOM  ; break;
        case TOKEN_IDENTIFIER: lhs = MAKE_OP_FROM_TOKEN(token, OP_IDENTIFIER, 0, TYPE_UNKNOWN); lot = LHS_OP_TYPE_ATOM  ; break;
        case TOKEN_PRINT     : lhs = MAKE_OP_FROM_TOKEN(token, OP_PRINT     , 0, TYPE_UNKNOWN); lot = LHS_OP_TYPE_ATOM  ; break;
        // Ops
        case TOKEN_MINUS       : op = MAKE_OP_FROM_TOKEN(token, OP_NEG    , 1, TYPE_UNKNOWN); lot = LHS_OP_TYPE_PREFIX; break;
        case TOKEN_BANG        : op = MAKE_OP_FROM_TOKEN(token, OP_NOT    , 1, TYPE_UNKNOWN); lot = LHS_OP_TYPE_PREFIX; break;
        case TOKEN_PLUS_PLUS   : op = MAKE_OP_FROM_TOKEN(token, OP_PRE_INC, 1, TYPE_UNKNOWN); lot = LHS_OP_TYPE_PREFIX; break;
        case TOKEN_MINUS_MINUS : op = MAKE_OP_FROM_TOKEN(token, OP_PRE_DEC, 1, TYPE_UNKNOWN); lot = LHS_OP_TYPE_PREFIX; break;
        // Special
        case TOKEN_LEFT_PAREN: lot = LHS_OP_TYPE_PARENS; break;
        default:
            fprintf(stderr, "Parser line %d: Could not find atomic expression.\n", __LINE__);
            exit(1);
    }
    switch (lot)
    {
        case LHS_OP_TYPE_ATOM:
            arrput(expr, lhs);
            break;

        case LHS_OP_TYPE_PREFIX:
            prefix_binding_power(op.op_type, &right_binding_power);
            rhs = parser_parse_expr_inner(parser, right_binding_power);

            arrput(expr, op);
            append_rhs_to_expr(&expr, &rhs);
            arrfree_and_set_null(rhs);
            break;

        case LHS_OP_TYPE_PARENS:
            token = parser_peek(parser);
            if (token.type == TOKEN_RIGHT_PAREN)
            {
                fprintf(stderr, "Parser line %d: Nothing found in parentheses.\n", __LINE__);
                exit(1);
            }
            rhs = parser_parse_expr_inner(parser, 0);

            append_rhs_to_expr(&expr, &rhs);
            arrfree_and_set_null(rhs);
            parser_next(parser);
            break;
        default:
            fprintf(stderr, "Parser line %d: Unknown LhsOpType value.\n", __LINE__);
            exit(1);
    }
    
    while (true)
    {
        bool should_break_loop = false;
        // Parse binary operator
        token = parser_peek(parser);
        switch (token.type)
        {
            case TOKEN_PLUS         : op = MAKE_OP_FROM_TOKEN(token, OP_ADD          , 2, TYPE_UNKNOWN); break;
            case TOKEN_MINUS        : op = MAKE_OP_FROM_TOKEN(token, OP_SUB          , 2, TYPE_UNKNOWN); break;
            case TOKEN_STAR         : op = MAKE_OP_FROM_TOKEN(token, OP_MUL          , 2, TYPE_UNKNOWN); break;
            case TOKEN_SLASH        : op = MAKE_OP_FROM_TOKEN(token, OP_DIV          , 2, TYPE_UNKNOWN); break;
            case TOKEN_PERCENT      : op = MAKE_OP_FROM_TOKEN(token, OP_MOD          , 2, TYPE_UNKNOWN); break;
            case TOKEN_AND          : op = MAKE_OP_FROM_TOKEN(token, OP_AND          , 2, TYPE_UNKNOWN); break;
            case TOKEN_OR           : op = MAKE_OP_FROM_TOKEN(token, OP_OR           , 2, TYPE_UNKNOWN); break;
            case TOKEN_EQUAL_EQUAL  : op = MAKE_OP_FROM_TOKEN(token, OP_EQUAL        , 2, TYPE_UNKNOWN); break;
            case TOKEN_BANG_EQUAL   : op = MAKE_OP_FROM_TOKEN(token, OP_NOT_EQUAL    , 2, TYPE_UNKNOWN); break;
            case TOKEN_GREATER      : op = MAKE_OP_FROM_TOKEN(token, OP_GREATER      , 2, TYPE_UNKNOWN); break;
            case TOKEN_LESS         : op = MAKE_OP_FROM_TOKEN(token, OP_LESS         , 2, TYPE_UNKNOWN); break;
            case TOKEN_GREATER_EQUAL: op = MAKE_OP_FROM_TOKEN(token, OP_GREATER_EQUAL, 2, TYPE_UNKNOWN); break;
            case TOKEN_LESS_EQUAL   : op = MAKE_OP_FROM_TOKEN(token, OP_LESS_EQUAL   , 2, TYPE_UNKNOWN); break;
            case TOKEN_DOT          : op = MAKE_OP_FROM_TOKEN(token, OP_ACCESS       , 2, TYPE_UNKNOWN); break;
            case TOKEN_COLON        : op = MAKE_OP_FROM_TOKEN(token, OP_CHAIN        , 2, TYPE_UNKNOWN); break;
            case TOKEN_EQUAL        : op = MAKE_OP_FROM_TOKEN(token, OP_ASSIGN       , 2, TYPE_UNKNOWN); break;
            case TOKEN_PLUS_EQUAL   : op = MAKE_OP_FROM_TOKEN(token, OP_ASSIGN_ADD   , 2, TYPE_UNKNOWN); break;
            case TOKEN_MINUS_EQUAL  : op = MAKE_OP_FROM_TOKEN(token, OP_ASSIGN_SUB   , 2, TYPE_UNKNOWN); break;
            case TOKEN_STAR_EQUAL   : op = MAKE_OP_FROM_TOKEN(token, OP_ASSIGN_MUL   , 2, TYPE_UNKNOWN); break;
            case TOKEN_PERCENT_EQUAL: op = MAKE_OP_FROM_TOKEN(token, OP_ASSIGN_MOD   , 2, TYPE_UNKNOWN); break;
            case TOKEN_PLUS_PLUS    : op = MAKE_OP_FROM_TOKEN(token, OP_POST_INC     , 1, TYPE_UNKNOWN); break;
            case TOKEN_MINUS_MINUS  : op = MAKE_OP_FROM_TOKEN(token, OP_POST_DEC     , 1, TYPE_UNKNOWN); break;
            case TOKEN_LEFT_SQUARE  : op = MAKE_OP_FROM_TOKEN(token, OP_INDEX        , 2, TYPE_UNKNOWN); break;
            case TOKEN_LEFT_PAREN   : op = MAKE_OP_FROM_TOKEN(token, OP_CALL         , 1, TYPE_UNKNOWN); break;
            default:
                should_break_loop = true;
        }
        if (should_break_loop) break;

        // Postfix
        if (postfix_binding_power(op.op_type, &left_binding_power))
        {
            if (left_binding_power < min_binding_power)
                break;
            parser_next(parser); // We probably want to do this now.

            if (op.op_type == OP_INDEX)
            {
                token = parser_peek(parser);
                if (token.type == TOKEN_RIGHT_SQUARE)
                {
                    fprintf(stderr, "Parser line %d: Nothing found in square braces.\n", __LINE__);
                    exit(1);
                }
                rhs = parser_parse_expr_inner(parser, 0);
                token = parser_next(parser); // Skip second bracket
                if (token.type == TOKEN_RIGHT_SQUARE)
                {
                    fprintf(stderr, "Parser line %d: Expected right square brace.\n", __LINE__);
                    exit(1);
                }

                arrins(expr, 0, op);
                append_rhs_to_expr(&expr, &rhs);
                arrfree_and_set_null(rhs);
            }
            else if (op.op_type == OP_CALL)
            {
                token = parser_peek(parser);
                if (token.type != TOKEN_RIGHT_PAREN)
                {
                    do
                    {
                        rhs = parser_parse_expr_inner(parser, 0);

                        append_rhs_to_expr(&expr, &rhs);
                        arrfree_and_set_null(rhs);

                        op.args++;

                        token = parser_next(parser); // Skip second bracket

                        if (token.type == TOKEN_RIGHT_PAREN)
                            break;
                        else if (token.type == TOKEN_COMMA)
                            continue;
                        else
                        {
                            fprintf(stderr, "Parser line %d: Function arguments have to be seperated by commas.\n", __LINE__);
                            exit(1);
                        }
                    }
                    while(true);
                }
                arrins(expr, 0, op);
            }
            else
            {
                arrins(expr, 0, op);
                //fprintf(stderr, "Parser line %d: Postfix parsing not implemented.\n", __LINE__);
                //exit(1);
            }

            continue;
        }

        // Infix
        if (infix_binding_power(op.op_type, &left_binding_power, &right_binding_power))
        {
            if (left_binding_power < min_binding_power)
                break;

            parser_next(parser);
            rhs = parser_parse_expr_inner(parser, right_binding_power);
            
            arrins(expr, 0, op);
            append_rhs_to_expr(&expr, &rhs);
            arrfree_and_set_null(rhs);
            continue;
        }

        break;
    }

    // print_expr_op(expr);
    return expr;

}
#undef MAKE_OP_FROM_TOKEN

void prefix_binding_power(ExprOpKind op_type, int* right)
{
    switch (op_type)
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

bool postfix_binding_power(ExprOpKind op_type, int* left)
{
    switch (op_type)
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
    ExprOpKind op_type,
    int* left,
    int* right)
{
    switch (op_type)
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

void append_rhs_to_expr(ExprOp** expr, ExprOp** rhs)
{
    int rhs_len = arrlen(*rhs);
    for (int i = 0; i < rhs_len; ++i)
    {
        ExprOp r = *rhs[i];
        arrput(*expr, r);
    }
}

// TODO: Implement later
// ExprOp* parser_parse_expr(Parser* parser)
// {
//     return ARENA_NULL;
// }

const char* show_op_type(ExprOpKind op)
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

void print_expr_op(ExprOp* op)
{
    int len = arrlen(op);
    printf("Expr (len = %d):\n", len);
    for (int i = 0; i < len; ++i)
    {
        ExprOp e = op[i];
        const char* type = show_op_type(e.op_type); 
        printf(
            "[%d:%d:%d]: '%.*s'\n",
            e.line   ,
            e.column ,
            e.length ,
            e.length ,
            e.literal
        );
    }
}

const char* stmt_type_name(StmtKind kind)
{
    switch (kind)
    {
        case STMT_ERR:               return "STMT_ERR";
        case STMT_LET_BARE:          return "STMT_LET_BARE";
        case STMT_LET_TYPE:          return "STMT_LET_TYPE";
        case STMT_LET_EXPR:          return "STMT_LET_EXPR";
        case STMT_LET_TYPE_AND_EXPR: return "STMT_LET_TYPE_AND_EXPR";
        case STMT_EXPR:              return "STMT_EXPR";
        case STMT_IF:                return "STMT_IF";
        case STMT_ELIF:              return "STMT_ELIF";
        case STMT_ELSE:              return "STMT_ELSE";
        case STMT_WHILE:             return "STMT_WHILE";
        case STMT_BREAK:             return "STMT_BREAK";
        case STMT_CONTINUE:          return "STMT_CONTINUE";
        case STMT_FN:                return "STMT_FN";
        case STMT_RETURN:            return "STMT_RETURN";
        case STMT_EMPTY:             return "STMT_EMPTY";
        default:                     return "UNKNOWN";
    }
}
