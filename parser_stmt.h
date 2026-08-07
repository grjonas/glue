#ifndef PARSER_STMT_H
#define PARSER_STMT_H

#include "parser_expr.h"

Stmt               * parser_parse_stmts                (Parser* parser);

// Stmt               * parser_parse_stmt                 (Parser* parser);

Stmt               * parser_parse_stmt_block           (Parser* parser);
Stmt               * parser_parse_stmt_let             (Parser* parser);
Stmt               * parser_parse_stmt_if              (Parser* parser, TokenType type);
Stmt               * parser_parse_stmt_while           (Parser* parser);
Stmt               * parser_parse_stmt_fn              (Parser* parser);
Stmt               * parser_parse_stmt_expr            (Parser* parser);
Stmt               * parser_parse_stmt_break           (Parser* parser);
Stmt               * parser_parse_stmt_continue        (Parser* parser);
Stmt               * parser_parse_stmt_return          (Parser* parser);
Stmt               * parser_parse_stmt_alias           (Parser* parser);
StmtTypeConstructor* parser_parse_stmt_type_constructor(Parser* parser);
Stmt               * parser_parse_stmt_type            (Parser* parser);

#endif
