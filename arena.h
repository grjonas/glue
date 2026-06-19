#ifndef ARENA_H
#define ARENA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ARENA_INITIAL_CAPACITY 256

typedef struct
{
    void*  body;
    size_t capacity;
    size_t size;
}
Arena;

Arena  init_arena              ();
void   free_arena              (Arena* arena);

void   arena_alloc_and_set_size(Arena* arena, size_t new_size);
size_t push_arena              (Arena* arena, size_t obj_size, const void* obj);
size_t pop_arena               (Arena* arena, size_t obj_size);

void* get_arena_element       (Arena* arena, size_t ptr);

#endif
