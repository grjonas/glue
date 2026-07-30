#ifndef COMPILE_ERROR_H
#define COMPILE_ERROR_H

/*
# Compiler error refactor

 */

#include "dependencies.h"

typedef enum   CompileErrorKind CompileErrorKind;
typedef struct CompileError     CompileError    ; 

enum CompileErrorKind
{
    ERROR_INFO   ,
    ERROR_WARNING,
    ERROR_ERROR  ,
};

struct CompileError
{
    CompileErrorKind kind;
    int line  ;
    int column;
    int length;

    const char* msg;
};

#endif
