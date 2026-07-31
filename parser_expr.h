#ifndef PARSER_EXPR_H
#define PARSER_EXPR_H

#include "parser_type_expr.h"

Expr* parser_parse_expr(Parser* parser);

Expr* parser_parse_expr_primary(Parser* parser);
Expr* parser_parse_expr_parens (Parser* parser);
Expr* parser_parse_expr_prefix (Parser* parser);
Expr* parser_parse_expr_index  (Parser* parser);
Expr* parser_parse_expr_fn     (Parser* parser);
Expr* parser_parse_expr_list   (Parser* parser);
Expr* parser_parse_expr_struct (Parser* parser);

ExprUnaryKind  get_prefix_operator(TokenType type, int* right_bp               );
ExprBinaryKind get_infix_operator (TokenType type, int* left_bp , int* right_bp);
ExprUnaryKind get_postfix_operator(TokenType type, int* right_bp               );

bool is_infix(TokenType type);
bool is_postfix(TokenType type, int* left_bp);

Expr** create_new_argument_list(Arena* arena, int old_argc, Expr** expr, Expr* lhs);

Expr* construct_assign_expr(Arena* arena, char* identifier, Expr* expr);

void print_expr_op(Expr* op);

#endif
