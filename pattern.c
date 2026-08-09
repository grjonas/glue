#include "pattern.h"

Pattern* create_pattern(Arena* arena)
{
    return (Pattern*) arena_push_empty(arena, sizeof(Pattern));
}
