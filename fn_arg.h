#ifndef FN_ARG_H
#define FN_ARG_H

typedef struct FnArg FnArg;

struct FnArg
{
    char    * identifier;
    Decl    * decl      ;
    TypeExpr* type      ;
};

#endif
