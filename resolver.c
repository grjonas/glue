#include "resolver.h"

Resolver resolver_init(Parser parser, Stmt* stmts)
{
    return (Resolver)
    {
        .txt             = parser.txt   ,
        .tokens          = parser.tokens,
        .stmts           = stmts        ,
        .arena           = parser.arena ,
        .tmp_type_arena  = NULL         ,
        .loop_depth      = 0            ,
        .inside_function = false        ,
        .fn_type         = NULL         ,
        .declarations    = NULL         ,
        .exprs           = NULL         ,
        .identifiers     = NULL         ,
        .errs            = NULL         ,
    };
}

void resolver_free(Resolver* resolver)
{
    free((char*)resolver->txt);
    arrfree(resolver->tokens);
    arena_free(&resolver->arena);
    arena_free(&resolver->tmp_type_arena);

    arrfree(resolver->declarations);
    arrfree(resolver->exprs       );
    arrfree(resolver->identifiers );
    arrfree(resolver->errs        );

    *resolver = (Resolver)
    {
        .txt             = NULL                    ,
        .tokens          = NULL                    ,
        .stmts           = NULL                    ,
        .arena           = resolver->arena         ,
        .tmp_type_arena  = resolver->tmp_type_arena,
        .loop_depth      = 0                       ,
        .inside_function = false                   ,
        .fn_type         = NULL                    ,
        .declarations    = NULL                    ,
        .exprs           = NULL                    ,
        .identifiers     = NULL                    ,
        .errs            = NULL                    ,
    };
}


