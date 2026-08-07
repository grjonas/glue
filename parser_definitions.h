#ifndef PARSER_DEFINITIONS_H
#define PARSER_DEFINITIONS_H

#include "parser.h"

Stmt    * parser_parse_stmt     (Parser* parser);
Expr    * parser_parse_expr     (Parser* parser);
TypeExpr* parser_parse_type_expr(Parser* parser);

FnArg* parser_parse_fn_arg(Parser* parser);

#endif
