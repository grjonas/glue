#ifndef ARENA_H
#define ARENA_H

#define ARENA_MINIMUM_BLOCK_CAPACITY 4096 // The size of a virtual memory page I believe
#define ARENA_MINIMUM_BLOCK_LIST_CAPACITY 4
#define ARENA_ALIGNMENT 8

// #define ALIGN8(x) (((x) + 7) & ~7)
// #define ALIGN(old, sz) ((old) % (sz) == 0 ? (old) : (((old)%(sz)+1) * (sz)))
// #define ALIGN(x, a) (((x) + ((a) - 1)) & ~((a) - 1))
// #define ALIGN(x, a) ((((x) + (a) - 1) / (a)) * (a))

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct
{
    // In bytes
    void*  body;
    size_t capacity;
    size_t size;
}
ArenaBlock;

typedef struct
{
    ArenaBlock* body;
    size_t capacity;
    size_t size;
}
Arena;

void arena_init(Arena* arena);
void arena_free(Arena* arena);

void* arena_push(Arena* arena, void* obj, size_t size);
void* arena_push_empty(Arena* arena, size_t size);

#endif
