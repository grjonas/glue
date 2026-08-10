#ifndef DIAGNOSTIC_H
#define DIAGNOSTIC_H

#include "dependencies.h"
#include "token.h"
#include "stmt.h"
#include "expr.h"
#include "type_expr.h"
#include "type.h"
#include "decl.h"
#include "print.h"

typedef struct Span                Span               ;
typedef enum   DiagnosticErrKind   DiagnosticErrKind  ;
typedef struct DiagnosticErr       DiagnosticErr      ;
typedef enum   DiagnosticKind      DiagnosticKind     ;
typedef struct Diagnostic          Diagnostic         ;
typedef struct DiagnosticComponent DiagnosticComponent;

struct Span
{
    const char* filename;
    int line;
    int column;
    int length;
};

enum DiagnosticErrKind
{
    // Scanner
    DIAGNOSTIC_ERR_REACHED_EOF                 ,
    DIAGNOSTIC_ERR_UNEXPECTED_CHAR             ,
    DIAGNOSTIC_ERR_UNEXPECTED_STR              ,
    DIAGNOSTIC_ERR_STR_NON_TERMINATING         ,
    DIAGNOSTIC_ERR_EXPECTED_DIGIT              ,

    // Parser
    DIAGNOSTIC_ERR_GENERIC                     ,
    DIAGNOSTIC_ERR_UNEXPECTED_TOKEN            ,
    DIAGNOSTIC_ERR_EXPECTED_TOKEN              ,
    DIAGNOSTIC_ERR_UNEXPECTED_PREFIX_OP        ,
    DIAGNOSTIC_ERR_STRUCT_DUPLICATE_IDENTIFIER ,
    DIAGNOSTIC_ERR_UNEXPECTED_PATTERN          ,

    // Resolver
    DIAGNOSTIC_ERR_BREAK_NOT_IN_LOOP                   ,
    DIAGNOSTIC_ERR_CONTINUE_NOT_IN_LOOP                ,
    DIAGNOSTIC_ERR_RETURN_NOT_IN_FN                    ,
    DIAGNOSTIC_ERR_RETURNS_DONT_MATCH                  ,
    DIAGNOSTIC_ERR_FAILED_TO_RESOLVE_IDENTIFIER        ,
    DIAGNOSTIC_ERR_FAILED_TO_RESOLVE_ACCESS_OP         ,
    DIAGNOSTIC_ERR_FAILED_TO_FIND_TYPE                 ,
    DIAGNOSTIC_ERR_PATTERN_FAILED_TO_RESOLVE_IDENTIFIER,
    DIAGNOSTIC_ERR_PATTERN_FAILED_TO_FIND_CONSTRUCTOR  ,

    // Inferer
    DIAGNOSTIC_ERR_UNIFY_FAILED                                       ,
    DIAGNOSTIC_ERR_EXPR_BINARY_ACCESS_OP_LEFT_KIND_NOT_STRUCT         ,
    DIAGNOSTIC_ERR_EXPR_BINARY_ACCESS_OP_STRUCT_DOES_NOT_CONTAIN_FIELD,
    DIAGNOSTIC_ERR_EXPR_FN_EXCESSIVE_ARGS                             ,
    DIAGNOSTIC_TYPE_ISNT_NUMERIC                                      ,
    DIAGNOSTIC_TYPE_ISNT_EQUALITY                                     ,
};

struct DiagnosticErr
{
    DiagnosticErrKind kind;
    Span span;

    union
    {
        struct
        {
            char charr;
        }
        unexpected_char;

        struct
        {
            int length;
            const char* str;
        }
        unexpected_str;

        struct
        {
            const char* file;
            int line;
        }
        generic;

        struct
        {
            TokenType* expected;
            int expected_count;
        }
        unexpected_token;

        struct
        {
            TokenType* expected;
            int expected_count;
        }
        expected_token;

        struct
        {
            TokenType token_type;
        }
        unexpected_prefix_op;

        struct
        {
            const char* identifier;
        }
        struct_duplicate_identifier;

        struct
        {
            const char* identifier;
        }
        failed_to_resolve_identifier;

        struct
        {
            const char* identifier;
        }
        type_expr_failed_to_find_type;

        struct
        {
            Type* left ;
            Type* right;
        }
        unify_failed;

        struct
        {
            const char* identifier;
        }
        pattern_failed_to_resolve_identifier;

        struct
        {
            const char* identifier;
        }
        pattern_failed_to_find_constructor;
    }
    err;
};

enum DiagnosticKind
{
    DIAGNOSTIC_ERR ,
    DIAGNOSTIC_WARN,
    DIAGNOSTIC_HELP,
    DIAGNOSTIC_INFO,
};

struct Diagnostic
{
    DiagnosticKind kind;
    union
    {
        DiagnosticErr err;
    }
    diagnostic;
};

struct DiagnosticComponent
{
    DYNAMIC_ARRAY(Diagnostic*) diagnostics;
    Arena arena;
};

DiagnosticComponent* diagnostic_component_init();
void  diagnostic_component_free(DiagnosticComponent** diagnostic_component);
void  diagnostic_component_push(DiagnosticComponent* diagnostic_component, Diagnostic diagnostic);
void  diagnostic_component_push_err(DiagnosticComponent* diagnostic_component, DiagnosticErr err);
void  diagnostic_component_print(DiagnosticComponent* diagnostic_component);
bool  diagnostic_component_is_empty(DiagnosticComponent* diagnostic_component);
int   diagnostic_component_get_msg_num(DiagnosticComponent* diagnostic_component);
char* diagnostic_component_add_identifier(DiagnosticComponent* diagnostic_component, const char* identifier, int length);

void  diagnostic_print(Diagnostic diagnostic);
void  diagnostic_err_print(DiagnosticErr err);

#endif
