#ifndef PARSER_TYPE_EXPR_H
#define PARSER_TYPE_EXPR_H

#include "parser_definitions.h"

// TypeExpr* parser_parse_type_expr          (Parser* parser);

TypeExpr* parser_parse_type_expr_primitive(Parser* parser);
TypeExpr* parser_parse_type_expr_list     (Parser* parser);
TypeExpr* parser_parse_type_expr_struct   (Parser* parser);
TypeExpr* parser_parse_type_expr_function (Parser* parser);
TypeExpr* parser_parse_type_expr_instance (Parser* parser);

TypeExpr* construct_primitive_type_expr(Arena* arena, TypeExprKind kind);

#endif
