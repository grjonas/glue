#include "dependencies.h"
#include "scanner.h"

// Returns a null-terminated string that has the file's contents.
// Needs to be freed.
int main()
{
    char* str = read_file("test.txt");
    Scanner scanner = init_scanner(str);
    scanner_scan_tokens(&scanner);
    printf("%s\n", str);
    for (int i = 0; i < arrlen(scanner.token_list); ++i)
    {
        Token t = scanner.token_list[i];
        const char* show = token_type_name(t.type);
        printf("[%d:%d:%d]: %s\n", t.line, t.column, t.length, show);
    }
    return 0;
}

