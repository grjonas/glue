#ifndef PATTERN_H
#define PATTERN_H

#include "dependencies.h"
#include "decl_definition.h"

typedef enum   PatternKind         PatternKind       ;
typedef enum   PatternLiteralKind  PatternLiteralKind;

typedef struct PatternWildcard     PatternWildcard   ;
typedef struct PatternLiteral      PatternLiteral    ;
typedef struct PatternVar          PatternVar        ;
typedef struct PatternResolvedVar  PatternResolvedVar;
typedef struct PatternConstructor  PatternConstructor;
typedef struct PatternApplication  PatternApplication;
// typedef struct PatternList         PatternList       ;

typedef struct Pattern     Pattern    ;

enum PatternKind
{
    PATTERN_WILDCARD    ,
    PATTERN_LITERAL     ,
    PATTERN_VAR         ,
    PATTERN_RESOLVED_VAR,
    PATTERN_CONSTRUCTOR ,
    PATTERN_APPLICATION , // Resolved constructor
    // PATTERN_LIST,
};

enum PatternLiteralKind
{
    PATTERN_LITERAL_NIL    ,
    PATTERN_LITERAL_TRUE   ,
    PATTERN_LITERAL_FALSE  ,
    PATTERN_LITERAL_INTEGER,
    PATTERN_LITERAL_NUMBER ,
    PATTERN_LITERAL_STRING ,
};

struct PatternWildcard
{
};

struct PatternLiteral
{
    PatternLiteralKind kind;
    union
    {
        const char* integer;
        const char* number ;
        const char* string ;
    }
    literal;
};

struct PatternVar
{
    const char* var;
};

struct PatternResolvedVar
{
    Decl* decl;
};

struct PatternConstructor
{
    const char* identifier;
    Pattern** argv;
    int       argc;
};

struct PatternApplication
{
    Decl* decl;
    Pattern** argv;
    int       argc;
};

// struct PatternList
// {
// };

struct Pattern
{
    PatternKind kind;
    int line  ;
    int column;
    int length;

    union
    {
        PatternWildcard    wildcard    ;
        PatternLiteral     literal     ;
        PatternVar         var         ;
        PatternResolvedVar resolved_var;
        PatternConstructor constructor ;
        PatternApplication application ;
        // PatternList        list;
    }
    pattern;
};

Pattern* create_pattern(Arena* arena);

#endif
