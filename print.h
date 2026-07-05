#ifndef PRINT_H
#define PRINT_H

#include "stmt.h"
#include "expr.h"
#include "type_expr.h"
#include "type.h"
#include "decl.h"

void stmt_print_inner(FILE* file, Stmt* stmt, int depth);
void stmt_print(FILE* file, Stmt* stmt);

void expr_print(FILE* file, Expr* expr);
void type_expr_print(FILE* file, TypeExpr* type_expr);
void decl_print(FILE* file, Decl* decl);
void type_print(FILE* file, Type* type);

#endif
