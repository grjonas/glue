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
    return arrlen(diagnostic_component->diagnostics) == 0;
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
    span_print(err.span);
    switch (err.kind)
    {
        // Parser
        case DIAGNOSTIC_ERR_GENERIC                     : printf(" DIAGNOSTIC_ERR_GENERIC\n"); return;
        case DIAGNOSTIC_ERR_UNEXPECTED_TOKEN            : printf(" DIAGNOSTIC_ERR_UNEXPECTED_TOKEN\n"); return;
        case DIAGNOSTIC_ERR_EXPECTED_TOKEN              : printf(" DIAGNOSTIC_ERR_EXPECTED_TOKEN\n"); return;
        case DIAGNOSTIC_ERR_UNEXPECTED_PREFIX_OP        : printf(" DIAGNOSTIC_ERR_UNEXPECTED_PREFIX_OP\n"); return;
        case DIAGNOSTIC_ERR_STRUCT_DUPLICATE_IDENTIFIER : printf(" DIAGNOSTIC_ERR_STRUCT_DUPLICATE_IDENTIFIER\n"); return;

        // Resolver
        case DIAGNOSTIC_ERR_BREAK_NOT_IN_LOOP           : printf(" DIAGNOSTIC_ERR_BREAK_NOT_IN_LOOP\n"); return;
        case DIAGNOSTIC_ERR_CONTINUE_NOT_IN_LOOP        : printf(" DIAGNOSTIC_ERR_CONTINUE_NOT_IN_LOOP\n"); return;
        case DIAGNOSTIC_ERR_RETURN_NOT_IN_FN            : printf(" DIAGNOSTIC_ERR_RETURN_NOT_IN_FN\n"); return;
        case DIAGNOSTIC_ERR_FAILED_TO_RESOLVE_IDENTIFIER: printf(" DIAGNOSTIC_ERR_FAILED_TO_RESOLVE_IDENTIFIER\n"); return;
        case DIAGNOSTIC_ERR_FAILED_TO_RESOLVE_ACCESS_OP : printf(" DIAGNOSTIC_ERR_FAILED_TO_RESOLVE_ACCESS_OP\n"); return;
        case DIAGNOSTIC_ERR_FAILED_TO_FIND_TYPE         : printf(" DIAGNOSTIC_ERR_FAILED_TO_FIND_TYPE\n"); return;

        // Inferer
        case DIAGNOSTIC_ERR_UNIFY_FAILED                                       :
            printf(" DIAGNOSTIC_ERR_UNIFY_FAILED\n");
            type_print(stdout, err.err.unify_failed.left );
            printf("\n    with\n");
            type_print(stdout, err.err.unify_failed.right);
            printf("\n");
            return;

        case DIAGNOSTIC_ERR_TYPE_FAILED_CONSTRAINT_NUMERIC                     : printf(" DIAGNOSTIC_ERR_TYPE_FAILED_CONSTRAINT_NUMERIC\n"); return;
        case DIAGNOSTIC_ERR_TYPE_FAILED_CONSTRAINT_EQUALITY                    : printf(" DIAGNOSTIC_ERR_TYPE_FAILED_CONSTRAINT_EQUALITY\n"); return;
        case DIAGNOSTIC_ERR_EXPR_BINARY_ARITHMETIC_CONSTRAINT_FAILED           : printf(" DIAGNOSTIC_ERR_EXPR_BINARY_ARITHMETIC_CONSTRAINT_FAILED\n"); return;
        case DIAGNOSTIC_ERR_EXPR_BINARY_EQUALITY_CONSTRAINT_FAILED             : printf(" DIAGNOSTIC_ERR_EXPR_BINARY_EQUALITY_CONSTRAINT_FAILED\n"); return;
        case DIAGNOSTIC_ERR_EXPR_BINARY_ACCESS_OP_LEFT_KIND_NOT_STRUCT         : printf(" DIAGNOSTIC_ERR_EXPR_BINARY_ACCESS_OP_LEFT_KIND_NOT_STRUCT\n"); return;
        case DIAGNOSTIC_ERR_EXPR_BINARY_ACCESS_OP_STRUCT_DOES_NOT_CONTAIN_FIELD: printf(" DIAGNOSTIC_ERR_EXPR_BINARY_ACCESS_OP_STRUCT_DOES_NOT_CONTAIN_FIELD\n"); return;
        default: printf("Default error"); return;
    }
    UNREACHABLE;
}
