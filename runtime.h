#ifndef RUNTIME_H
#define RUNTIME_H

#include "encoder.h"

typedef struct
{
    Obj** objs ;
    Obj** stack;
    ObjFn* code;
}
Runtime;

#endif
