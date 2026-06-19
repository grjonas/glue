#include "arena.h"

Arena init_arena()
{
    return (Arena)
    {
        .body     = NULL,
        .capacity = 0,
        .size     = 0
    };
}

void free_arena(Arena* arena)
{
    free(arena->body);
    *arena = (Arena)
    {
        .body     = NULL,
        .capacity = 0   ,
        .size     = 0
    };
}

// Fix overflows
void arena_alloc_and_set_size(Arena* arena, size_t new_size)
{
    if (new_size > arena->size)
    {
        size_t new_capacity = arena->capacity == 0 ? ARENA_INITIAL_CAPACITY : arena->capacity * 2;
        if (new_capacity < arena->capacity)
        {
            fprintf(stderr, "Integer overflow in %s:%d.\n", __FILE__, __LINE__);
            exit(1);
        }
        for (; new_size > new_capacity; new_capacity *= 2);

        void* new_body = realloc(arena->body, new_capacity);
        if (new_body == NULL)
        {
            fprintf(stderr, "Arena failed to allocate memory in %s:%d.\n", __FILE__, __LINE__);
            exit(1);
        }

        *arena = (Arena)
        {
            .body     = new_body    ,
            .capacity = new_capacity,
            .size     = new_size
        };
    }
    else if (new_size < arena->size / 2 && arena->size / 2 >= ARENA_INITIAL_CAPACITY)
    {
        size_t new_capacity = arena->size / 4;
        // 0, 1, 2, 4, 8, ...
        for (; new_size < new_capacity && new_capacity >= ARENA_INITIAL_CAPACITY; new_capacity /= 2);
        new_capacity *= 2;

        void* new_body = realloc(arena->body, new_capacity);
        if (new_body == NULL)
        {
            fprintf(stderr, "Arena failed to allocate memory in %s:%d.\n", __FILE__, __LINE__);
            exit(1);
        }

        *arena = (Arena)
        {
            .body     = new_body    ,
            .capacity = new_capacity,
            .size     = new_size
        };
    }
    else
    {
        arena->size = new_size;
    }
}

// returns new offset
size_t push_arena(Arena* arena, size_t obj_size, const void* obj)
{
    size_t new_size   = arena->size + obj_size;
    size_t old_offset = arena->size;

    if (new_size < arena->size)
    {
        fprintf(stderr, "Integer overflow in %s:%d.\n", __FILE__, __LINE__);
        exit(1);
    }
    arena_alloc_and_set_size(arena, new_size);
    if (arena->body == NULL || obj == NULL)
    {
        fprintf(stderr, "Arena pointer or object pointer is NULL in %s:%d.\n", __FILE__, __LINE__);
        exit(1);
    }
    memcpy(arena->body, obj, obj_size);

    return old_offset;
}

// returns new offset
size_t pop_arena(Arena* arena, size_t obj_size)
{
    size_t new_size   = arena->size - obj_size;
    size_t old_offset = arena->size;

    if (new_size < arena->size)
    {
        fprintf(stderr, "Integer underflow in %s:%d.\n", __FILE__, __LINE__);
        exit(1);
    }
    arena_alloc_and_set_size(arena, new_size);

    return old_offset;
}

void* get_arena_element(Arena* arena, size_t ptr)
{
    return &(((char*) arena->body )[ptr]);
}
