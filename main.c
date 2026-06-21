#include "dependencies.h"
#include "scanner.h"
#include "parser.h"

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

    Parser parser = init_parser(scanner);

    // ExprOp* expr = parser_parse_expr(&parser);
    // print_expr_op(expr);

    // Stmt* stmt = parser_parse_stmt(&parser);
    parser_parse_stmt(&parser);

    return 0;
}
