#include "arena.h"
#include "dependencies.h"
#include "stmt.h"
#include "type_expr.h"
#include "expr.h"
#include "decl.h"
#include "type.h"
#include "diagnostic.h"
#include "print.h"
#include "scanner.h"
#include "parser.h"
#include "parser_stmt.h"
#include "parser_type_expr.h"
#include "parser_expr.h"
#include "resolver.h"
#include "inferer.h"

// Returns a null-terminated string that has the file's contents.
// Needs to be freed.
int main(int argc, char** argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Not enough arguments\n");
        exit(1);
    }

    Scanner scanner = init_scanner(argv[argc - 1]);
    scanner_scan_tokens(&scanner);

    printf("%s\n", scanner.init);

    for (int i = 0; i < arrlen(scanner.token_list); ++i)
    {
        Token t = scanner.token_list[i];

        printf(
            "[%d:%d:%d]: %-20s '%.*s'\n",
            t.line,
            t.column,
            t.length,
            token_type_name(t.type),
            t.length,
            t.start
        );
    }

    Parser parser;

    parser = init_parser(scanner);

    // ExprOp* expr = parser_parse_expr(&parser);
    // print_expr_op(expr);

    // Stmt* stmt = parser_parse_stmt(&parser);
    Stmt* stmt = parser_parse_stmts(&parser);

    diagnostic_component_print(parser.diagnostic_component);
    if (diagnostic_component_is_empty(parser.diagnostic_component))
        exit(1);

    Resolver resolver = resolver_init(&parser, stmt);

    resolver_resolve_stmt(&resolver);

    diagnostic_component_print(resolver.diagnostic_component);
    if (diagnostic_component_is_empty(resolver.diagnostic_component))
        exit(1);

    arena_print_memory_usage(&resolver.arena);

    FILE* file = stdout;
    fprintf(file, "Declarations:\n");
    for (int i = 0; i < arrlen(resolver.declarations); ++i)
    {
        Decl* d = resolver.declarations[i];
        fprintf(file, "    ");
        decl_print(file, d);
        fprintf(file, "\n");
    }
    stmt_print(file, resolver.stmts);

    Inferer inferer = inferer_init(&resolver);

    inferer_infer_stmt(&inferer, inferer.stmts);

    diagnostic_component_print(inferer.diagnostic_component);
    if (diagnostic_component_is_empty(inferer.diagnostic_component))
        exit(1);

    fprintf(file, "Declarations:\n");
    for (int i = 0; i < arrlen(inferer.declarations); ++i)
    {
        Decl* d = inferer.declarations[i];
        fprintf(file, "    ");
        decl_print(file, d);
        fprintf(file, "\n");
    }
    stmt_print(file, inferer.stmts);

    arena_print_memory_usage(&inferer.arena);
    arena_print_memory_usage(&inferer.type_arena);

    inferer_free(&inferer);

    return 0;
}
