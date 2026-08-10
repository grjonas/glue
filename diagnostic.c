#include "diagnostic.h"

DiagnosticComponent* diagnostic_component_init()
{
    DiagnosticComponent diagnostic_component;
    Arena arena;
    DiagnosticComponent* ptr;

    arena_init(&arena);
    diagnostic_component = (DiagnosticComponent)
    {
        .diagnostics = NULL ,
        .arena       = arena,
    };

    ptr = (DiagnosticComponent*) malloc(sizeof(DiagnosticComponent));
    assert(ptr != NULL);
    *ptr = diagnostic_component;

    return ptr;
}

void diagnostic_component_free(DiagnosticComponent** diagnostic_component)
{
    assert(diagnostic_component != NULL);
    assert(*diagnostic_component != NULL);

    arrfree((*diagnostic_component)->diagnostics);
    arena_free(&(*diagnostic_component)->arena);
    free(*diagnostic_component);

    *diagnostic_component = NULL;
}

void diagnostic_component_push(DiagnosticComponent* diagnostic_component, Diagnostic diagnostic)
{
    assert(diagnostic_component != NULL);

    arrput(diagnostic_component->diagnostics, diagnostic);
}

void diagnostic_component_push_err(DiagnosticComponent* diagnostic_component, DiagnosticErr err)
{
    assert(diagnostic_component != NULL);

    diagnostic_component_push(diagnostic_component, (Diagnostic)
    {
        .kind = DIAGNOSTIC_ERR,
        .diagnostic.err  = err,
    });
}

void diagnostic_component_print(DiagnosticComponent* diagnostic_component)
{
    assert(diagnostic_component != NULL);

    for (int i = 0; i < arrlen(diagnostic_component->diagnostics); ++i)
    {
        Diagnostic diagnostic = diagnostic_component->diagnostics[i];
        diagnostic_print(diagnostic);
    }
}

bool diagnostic_component_is_empty(DiagnosticComponent* diagnostic_component)
{
    int length = arrlen(diagnostic_component->diagnostics);
    return length == 0;
}

int diagnostic_component_get_msg_num(DiagnosticComponent* diagnostic_component)
{
    return arrlen(diagnostic_component->diagnostics);
}

char* diagnostic_component_add_identifier(DiagnosticComponent* diagnostic_component, const char* identifier, int length)
{
    char* pushed_identfier = (char*) arena_push_empty(&diagnostic_component->arena, (length + 1) * sizeof(char));
    memcpy(pushed_identfier, identifier, length);
    return pushed_identfier;
}

void span_print(Span span)
{
    printf("[%d:%d:%d]", span.line, span.column, span.length);
}

void diagnostic_print(Diagnostic diagnostic)
{
    switch (diagnostic.kind)
    {
        case DIAGNOSTIC_ERR : diagnostic_err_print(diagnostic.diagnostic.err); return;
        case DIAGNOSTIC_WARN: UNREACHABLE;
        case DIAGNOSTIC_HELP: UNREACHABLE;
        case DIAGNOSTIC_INFO: UNREACHABLE;
    }
    UNREACHABLE;
}

// TODO: Rewrite this function later to be prettier.
void diagnostic_err_print(DiagnosticErr err)
{
#define GENERIC_PRINT(error) case (error) : printf(" (error)\n"); return;

    span_print(err.span);
    switch (err.kind)
    {
        case DIAGNOSTIC_ERR_REACHED_EOF                 : printf(" DIAGNOSTIC_ERR_REACHED_EOF\n"); return;
        case DIAGNOSTIC_ERR_UNEXPECTED_CHAR             : printf(" DIAGNOSTIC_ERR_UNEXPECTED_CHAR\n"); return;
        case DIAGNOSTIC_ERR_UNEXPECTED_STR              : printf(" DIAGNOSTIC_ERR_UNEXPECTED_STR\n"); return;
        case DIAGNOSTIC_ERR_STR_NON_TERMINATING         : printf(" DIAGNOSTIC_ERR_STR_NON_TERMINATING\n"); return;
        case DIAGNOSTIC_ERR_EXPECTED_DIGIT              : printf(" DIAGNOSTIC_ERR_EXPECTED_DIGIT\n"); return;

        // Parser
        case DIAGNOSTIC_ERR_GENERIC                     : printf(" DIAGNOSTIC_ERR_GENERIC\n"); return;
        case DIAGNOSTIC_ERR_UNEXPECTED_TOKEN            :
        {
            TokenType* expected = err.err.unexpected_token.expected      ;
            int  expected_count = err.err.unexpected_token.expected_count;

            printf(" DIAGNOSTIC_ERR_UNEXPECTED_TOKEN\n");

            for (int i = 0; i < expected_count; ++i)
            {
                printf("    %s\n", show_token_type(expected[i]));
            }
            return;
        }

        case DIAGNOSTIC_ERR_EXPECTED_TOKEN              : printf(" DIAGNOSTIC_ERR_EXPECTED_TOKEN\n"); return;
        case DIAGNOSTIC_ERR_UNEXPECTED_PREFIX_OP        : printf(" DIAGNOSTIC_ERR_UNEXPECTED_PREFIX_OP\n"); return;
        case DIAGNOSTIC_ERR_STRUCT_DUPLICATE_IDENTIFIER : printf(" DIAGNOSTIC_ERR_STRUCT_DUPLICATE_IDENTIFIER\n"); return;
        case DIAGNOSTIC_ERR_UNEXPECTED_PATTERN          : printf(" DIAGNOSTIC_ERR_UNEXPECTED_PATTERN\n"); return;

        // Resolver
        case DIAGNOSTIC_ERR_BREAK_NOT_IN_LOOP                   : printf(" DIAGNOSTIC_ERR_BREAK_NOT_IN_LOOP\n"); return;
        case DIAGNOSTIC_ERR_CONTINUE_NOT_IN_LOOP                : printf(" DIAGNOSTIC_ERR_CONTINUE_NOT_IN_LOOP\n"); return;
        case DIAGNOSTIC_ERR_RETURN_NOT_IN_FN                    : printf(" DIAGNOSTIC_ERR_RETURN_NOT_IN_FN\n"); return;

        case DIAGNOSTIC_ERR_RETURNS_DONT_MATCH                  : printf(" DIAGNOSTIC_ERR_RETURNS_DONT_MATCH\n"); return;
        case DIAGNOSTIC_ERR_FAILED_TO_RESOLVE_IDENTIFIER        : printf(" DIAGNOSTIC_ERR_FAILED_TO_RESOLVE_IDENTIFIER\n"); return;
        case DIAGNOSTIC_ERR_FAILED_TO_RESOLVE_ACCESS_OP         : printf(" DIAGNOSTIC_ERR_FAILED_TO_RESOLVE_ACCESS_OP\n"); return;
        case DIAGNOSTIC_ERR_FAILED_TO_FIND_TYPE                 : printf(" DIAGNOSTIC_ERR_FAILED_TO_FIND_TYPE\n"); return;
        case DIAGNOSTIC_ERR_PATTERN_FAILED_TO_RESOLVE_IDENTIFIER: printf(" DIAGNOSTIC_ERR_PATTERN_FAILED_TO_RESOLVE_IDENTIFIER\n"); return;
        case DIAGNOSTIC_ERR_PATTERN_FAILED_TO_FIND_CONSTRUCTOR  : printf(" DIAGNOSTIC_ERR_PATTERN_FAILED_TO_FIND_CONSTRUCTOR\n"); return;

        // Inferer
        case DIAGNOSTIC_ERR_UNIFY_FAILED                                       :
            printf(" DIAGNOSTIC_ERR_UNIFY_FAILED\n");
            type_print(stdout, err.err.unify_failed.left );
            printf("\n    with\n");
            type_print(stdout, err.err.unify_failed.right);
            printf("\n");
            return;

        case DIAGNOSTIC_ERR_EXPR_BINARY_ACCESS_OP_LEFT_KIND_NOT_STRUCT         : printf(" DIAGNOSTIC_ERR_EXPR_BINARY_ACCESS_OP_LEFT_KIND_NOT_STRUCT\n"); return;
        case DIAGNOSTIC_ERR_EXPR_BINARY_ACCESS_OP_STRUCT_DOES_NOT_CONTAIN_FIELD: printf(" DIAGNOSTIC_ERR_EXPR_BINARY_ACCESS_OP_STRUCT_DOES_NOT_CONTAIN_FIELD\n"); return;
        case DIAGNOSTIC_ERR_EXPR_FN_EXCESSIVE_ARGS                             : printf(" DIAGNOSTIC_ERR_EXPR_FN_EXCESSIVE_ARGS\n"); return;
        case DIAGNOSTIC_TYPE_ISNT_NUMERIC                                      : printf(" DIAGNOSTIC_TYPE_ISNT_NUMERIC\n"); return;
        case DIAGNOSTIC_TYPE_ISNT_EQUALITY                                     : printf(" DIAGNOSTIC_TYPE_ISNT_EQUALITY\n"); return;
    }
    UNREACHABLE;

#undef GENERIC_PRINT
}
