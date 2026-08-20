#ifndef ENCODER_H
#define ENCODER_H

#include "inferer.h"

typedef uint8_t Byte;

typedef enum
{
    // Stack manipulation
    OP_CODE_PUSH_NIL  , // _ -> {:nil}
    OP_CODE_PUSH_TRUE , // _ -> {:bool}
    OP_CODE_PUSH_FALSE, // _ -> {:false}
    OP_CODE_PUSH      , // [:primitive] -> {:primitive}
    OP_CODE_POP       , // {:obj} -> _
    OP_CODE_DUP       , // {:obj} -> {:obj}, {:obj}
    OP_CODE_STORE     , // {:obj}, {stack_index} -> _
    OP_CODE_LOAD      , // {stack_index} -> {:obj} # Object is removed from the location it's taken from.
    OP_CODE_COPY      , // {stack_index} -> {:obj}
    OP_CODE_FREE      , // {:obj} -> _

    OP_CODE_NOT       , // {:bool} -> {:bool}
    OP_CODE_AND       , // {:bool}, {:bool} -> {:bool}
    OP_CODE_OR        , // {:bool}, {:bool} -> {:bool}

    OP_CODE_NEG       , // Num(a) => {a} -> {a},
    OP_CODE_ADD       , // Num(a) => {a}, {a} -> {a},
    OP_CODE_SUB       , // Num(a) => {a}, {a} -> {a},
    OP_CODE_MUL       , // Num(a) => {a}, {a} -> {a},
    OP_CODE_DIV       , // Num(a) => {a}, {a} -> {a},
    OP_CODE_MOD       , // Num(a) => {a}, {a} -> {a},

    OP_CODE_EQL       , // Eql(a) => {a}, {a} -> {:bool}
    OP_CODE_NEQ       , // Eql(a) => {a}, {a} -> {:bool}
    OP_CODE_GR        , // Num(a) => {a}, {a} -> {:bool}
    OP_CODE_LS        , // Num(a) => {a}, {a} -> {:bool}
    OP_CODE_GRE       , // Num(a) => {a}, {a} -> {:bool}
    OP_CODE_LSE       , // Num(a) => {a}, {a} -> {:bool}

    OP_CODE_NEW_LIST  , // {...:obj}, {:nat} -> {:list}
    OP_CODE_NEW_STRUCT, // {...:obj}, {:nat} -> {:struct}
    OP_CODE_TCALL     , // {...:obj}, {:obj}, {:nat} -> {:obj}
    OP_CODE_CALL      , // {...:obj}, {:obj}, {:nat} -> {:obj}
    OP_CODE_ACCESS    , // {:derivative}, {:nat} -> {:obj}

    // OP_CODE_JMP_FRWD  , // {:bool}, {:nat} -> _, # goes forward  n lines if {:bool} == true
    // OP_CODE_JMP_BACK  , // {:bool}, {:nat} -> _, # goes backward n lines if {:bool} == true
    // OP_CODE_UJMP_FRWD , // {:nat} -> _, # goes forward  n lines
    // OP_CODE_UJMP_BACK , // {:nat} -> _, # goes backward n lines
    OP_CODE_GOTO      , // {:bool}, {:nat} -> _, # goes to line n
    OP_CODE_UGOTO     , // {:nat} -> _, # goes to line n
}
OpCode;

typedef enum
{
    // Primitives
    OBJ_BOOL  ,
    OBJ_NIL   ,
    OBJ_NAT   ,
    OBJ_INT   ,
    OBJ_REAL  ,
    OBJ_STRING,

    // Derivatives
    OBJ_LIST            ,
    OBJ_STRUCT          ,
    OBJ_TYPE_CONSTRUCTOR,

    OBJ_FN     ,
    OBJ_CLOSURE,
}
ObjKind;

typedef struct
{
    ObjKind kind;
    unsigned int size;
    Byte  data[];
}
Obj;

typedef struct
{
    unsigned int size;
    Byte data[];
}
ObjNat;

typedef struct
{
    unsigned int size;
    Byte data[];
}
ObjInt;

typedef struct
{
    unsigned int size;
    Byte data[];
}
ObjReal;

typedef struct
{
    unsigned int size;
    Byte data[];
}
ObjString;

typedef struct
{
    unsigned int size;
    Byte data[];
}
ObjList;

typedef struct
{
    unsigned int size;
    Byte data[];
}
ObjStruct;

typedef struct
{
    unsigned int arity ;
    unsigned int length;
    Byte code[];
}
ObjFn;

typedef struct
{
    ObjFn fn;
}
ObjClosure;

typedef struct
{
    unsigned int type_id;
    unsigned int cons_id; // Constructor id
    unsigned int    size;
    Byte data[];
}
ObjTypeConstructor;

typedef struct
{
    Obj**  objs;
    ObjFn* code;
}
Encoder;

extern Encoder encoder_init(Inferer* inferer);
extern void    encoder_free(Encoder* encoder);

extern void encoder_encode_expr(Encoder* encoder, Expr* expr, DYNAMIC_ARRAY(Byte*)* code_ref);
extern void encoder_encode_stmt(Encoder* encoder, Stmt* stmt);

#endif
