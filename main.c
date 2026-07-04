#include "dependencies.h"
#include "scanner.h"
#include "parser.h"
#include "stmt.h"
#include "type.h"
#include "expr.h"
#include "resolver.h"
#include "print.h"

// Returns a null-terminated string that has the file's contents.
// Needs to be freed.
int main(int argc, char** argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Not enough arguments\n");
        exit(1);
    }

    char* str = read_file(argv[argc - 1]);

    printf("%s\n", str);

    Scanner scanner = init_scanner(str);
    scanner_scan_tokens(&scanner);

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

    int arr_len = arrlen(parser.errs);
    for (int i = 0; i < arr_len; ++i)
    {
        CompileError t = *(parser.errs[i]);

        fprintf(
            stderr,
            "[%d:%d:%d]: %s\n",
            t.line  ,
            t.column,
            t.length,
            t.msg   
        );
    }
    if (arr_len > 0)
        exit(1);

    Resolver resolver = resolver_init(&parser, stmt);

    resolver_resolve_stmt(&resolver);

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

    resolver_free(&resolver);

    return 0;
}
