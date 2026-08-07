#ifndef PARSER_EXPR_H
#define PARSER_EXPR_H

#include "parser_type_expr.h"

// Expr* parser_parse_expr(Parser* parser);

Expr* parser_parse_expr_primary(Parser* parser);
Expr* parser_parse_expr_parens (Parser* parser);
Expr* parser_parse_expr_prefix (Parser* parser);
Expr* parser_parse_expr_index  (Parser* parser);
Expr* parser_parse_expr_fn     (Parser* parser);
Expr* parser_parse_expr_list   (Parser* parser);
Expr* parser_parse_expr_struct (Parser* parser);
Expr* parser_parse_expr_lambda (Parser* parser);

#endif
