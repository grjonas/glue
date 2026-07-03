#include "inferer.h"

Inferer inferer_init(Resolver* resolver)
{
    arrfree(resolver->context     );
    arrfree(resolver->errs        );

    *resolver = (Resolver)
    {
        .txt             = NULL                    ,
        .tokens          = NULL                    ,
        .stmts           = NULL                    ,
        .arena           = resolver->arena         ,
        .loop_depth      = 0                       ,
        .inside_function = false                   ,
        .context         = NULL                    ,
        .declarations    = NULL                    ,
        .identifiers     = NULL                    ,
        .errs            = NULL                    ,
    };

    return (Inferer)
    {
        .txt         = resolver->txt         ,
        .tokens      = resolver->tokens      ,
        .stmts       = resolver->stmts       ,
        .decl        = resolver->declarations,
        .identifiers = identifiers           ,
        .arena       = arena                 ,
        .errs        = NULL                  ,
    };
}

void    inferer_free(Inferer* inferer  )
{
    free((char*) resolver->txt);
    arrfree(resolver->tokens);
    arrfree(resolver->stmts);
    arrfree(resolver->declarations);
    arrfree(resolver->identifiers);
    arena_free(&resolver->arena);
    arrfree(resolver->errs);

    *inferer = (Inferer)
    {
        .txt          = NULL,
        .tokens       = NULL,
        .stmts        = NULL,
        .declarations = NULL,
        .identifiers  = NULL,
        .errs         = NULL,
    };
}
