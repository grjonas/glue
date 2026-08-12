#include "pattern.h"

extern Pattern* create_pattern(Arena* arena)
{
    return (Pattern*) arena_push_empty(arena, sizeof(Pattern));
}

extern Type* pattern_get_literal_type(PatternLiteral literal)
{
    switch (literal.kind)
    {
        case PATTERN_LITERAL_NIL    : return builtin_type_nil   ;
        case PATTERN_LITERAL_TRUE   : return builtin_type_bool  ;
        case PATTERN_LITERAL_FALSE  : return builtin_type_bool  ;
        case PATTERN_LITERAL_INTEGER: return builtin_type_int   ;
        case PATTERN_LITERAL_NUMBER : return builtin_type_real  ;
        case PATTERN_LITERAL_STRING : return builtin_type_string;
    }
    UNREACHABLE;
}
