#ifndef INFERER_H
#define INFERER_H

#include "resolver.h"

typedef struct Inferer Inferer

struct Inferer
{
    // Inputs
    const char* txt;
    Token* tokens;
    Stmt*  stmts;
    Decl** declarations; // Holds ALL scanned declarations
    char** identifiers;

    // Memory-management
    Arena  arena;

    // Misc. state

    // Outputs

    // Errs
    CompileError** errs;
};

Inferer inferer_init(Resolver* resolver);
void    inferer_free(Inferer* inferer  );

#endif
