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

    FILE* file = stdout;

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
            show_token_type(t.type),
            t.length,
            t.start
        );
    }

    if (!diagnostic_component_is_empty(scanner.diagnostic_component))
    {
        diagnostic_component_print(scanner.diagnostic_component);
        exit(1);
    }

    printf("================================================================================\n");

    Parser parser;

    parser = init_parser(&scanner);

    Stmt* stmt = parser_parse_stmts(&parser);

    if (!diagnostic_component_is_empty(parser.diagnostic_component))
    {
        diagnostic_component_print(parser.diagnostic_component);
        exit(1);
    }

    stmt_print(file, stmt);

    printf("================================================================================\n");

    Resolver resolver = resolver_init(&parser, stmt);

    resolver_resolve_stmt(&resolver);

    if (!diagnostic_component_is_empty(resolver.diagnostic_component))
    {
        diagnostic_component_print(resolver.diagnostic_component);
        exit(1);
    }

    arena_print_memory_usage(&resolver.arena);

    fprintf(file, "Declarations:\n");
    for (int i = 0; i < arrlen(resolver.declarations); ++i)
    {
        Decl* d = resolver.declarations[i];
        fprintf(file, "    ");
        decl_print(file, d);
        fprintf(file, "\n");
    }
    stmt_print(file, resolver.stmts);

    printf("================================================================================\n");

    Inferer inferer = inferer_init(&resolver);

    inferer_infer_stmt(&inferer, inferer.stmts);

    if (!diagnostic_component_is_empty(inferer.diagnostic_component))
    {
        if (true)
        {
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

        }
        printf("Inference errors:\n");
        diagnostic_component_print(inferer.diagnostic_component);
        exit(1);
    }

    fprintf(file, "Declarations:\n");
    for (int i = 0; i < arrlen(inferer.declarations); ++i)
    {
        Decl* d = inferer.declarations[i];
        fprintf(file, "    ");
        decl_print(file, d);
        fprintf(file, "\n");
    }
    stmt_print(file, inferer.stmts);

    printf("================================================================================\n");

    arena_print_memory_usage(&inferer.arena);
    arena_print_memory_usage(&inferer.type_arena);

    inferer_free(&inferer);

    return 0;
}
