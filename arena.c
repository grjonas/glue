#include "arena.h"

#define ALIGN(x, a) (((x) + ((a) - 1)) & ~((size_t)((a) - 1)))

void arena_init(Arena* arena)
{
    *arena = (Arena)
    {
        .body     = NULL,
        .capacity = 0,
        .size     = 0,
    };
}

void arena_free(Arena* arena)
{
    if (!arena || !arena->body) return;

    for (size_t i = 0; i < arena->size; i++)
    {
        free(arena->body[i].body);
    }

    free(arena->body);

    *arena = (Arena){0};
}

ArenaBlock* arena_get_block_with_enough_memory(Arena* arena, size_t size)
{
    if (!arena || arena->size == 0)
        return NULL;

    size_t required = ALIGN(size, ARENA_ALIGNMENT);

    for (size_t i = 0; i < arena->size; i++)
    {
        ArenaBlock* block = &arena->body[i];

        if (block->size > block->capacity)
        {
            fprintf(stderr, "Arena corruption detected\n");
            exit(1);
        }

        size_t available = block->capacity - block->size;

        if (available >= required)
            return block;
    }

    return NULL;
}

void* arena_block_push(ArenaBlock* block, void* obj, size_t size)
{
    if (block == NULL)
    {
        fprintf(stderr, "[%s:%d] Cannot push to a NULL block.\n", __FILE__, __LINE__);
        exit(1);
    }

    void* memory = NULL;
    size_t aligned_size = ALIGN(block->size, ARENA_ALIGNMENT);
    size_t new_size = aligned_size + size;

    if (new_size < size)
    {
        fprintf(stderr, "[%s:%d] Unsigned integer overflow error.\n", __FILE__, __LINE__);
        exit(1);
    }

    if (new_size > block->capacity)
    {
        fprintf(stderr, "[%s:%d] New block size exceeds capacity.\n", __FILE__, __LINE__);
        exit(1);
    }

    memory = (char*)block->body + aligned_size;
    block->size = new_size;
    memcpy(memory, obj, size);
    return memory;
}

ArenaBlock* arena_push_block_to_list(Arena* arena, ArenaBlock block)
{
    if (!arena)
        exit(1);

    if (arena->size == arena->capacity)
    {
        size_t new_cap = arena->capacity ? arena->capacity * 2 : ARENA_MINIMUM_BLOCK_LIST_CAPACITY;

        if (new_cap < arena->capacity)
            exit(1);

        void* mem = realloc(arena->body, new_cap * sizeof(ArenaBlock));
        if (!mem) exit(1);

        arena->body = mem;
        arena->capacity = new_cap;
    }

    arena->body[arena->size] = block;
    return &arena->body[arena->size++];
}

ArenaBlock* arena_allocate_new_block(Arena* arena, size_t size)
{
    if (arena == NULL)
    {
        fprintf(stderr, "[%s:%d] Cannot allocate memory to a NULL arena.\n", __FILE__, __LINE__);
        exit(1);
    }

    ArenaBlock block;
    void* memory = NULL;
    size_t block_capacity = ARENA_MINIMUM_BLOCK_CAPACITY < size ? size : ARENA_MINIMUM_BLOCK_CAPACITY;

    memory = calloc(block_capacity, sizeof(char));
    if (memory == NULL)
    {
        fprintf(stderr, "[%s:%d] Unable to allocate memory.\n", __FILE__, __LINE__);
        exit(1);
    }

    block = (ArenaBlock)
    {
        .body     = memory        ,
        .capacity = block_capacity,
        .size     = 0             ,
    };

    return arena_push_block_to_list(arena, block);
}

// int compare_arena_block_leftover_sizes(const void* a, const void* b)
// {
//     // Not checking for NULL, because the program will crash anyway :)
//     ArenaBlock block_a = *(ArenaBlock*)a;
//     ArenaBlock block_b = *(ArenaBlock*)b;
// 
//     size_t leftover_a  = block_a.capacity - block_a.size;
//     size_t leftover_b  = block_b.capacity - block_b.size;
// 
//     if (leftover_a == leftover_b)
//         return 0;
// 
//     return leftover_a < leftover_b ? 1 : -1;
// }

// void arena_reorder_blocks(Arena* arena)
// {
//     // Sorts descending (Unless I messed something up of course)
//     qsort(arena->body, arena->size, sizeof(ArenaBlock), compare_arena_block_leftover_sizes);
// }

void arena_make_sure_block_list_is_initialized(Arena* arena)
{
    if (arena == NULL)
    {
        fprintf(stderr, "[%s:%d] Cannot allocate memory to a NULL arena.\n", __FILE__, __LINE__);
        exit(1);
    }

    if (arena->capacity == 0)
    {
        void* memory = calloc(ARENA_MINIMUM_BLOCK_LIST_CAPACITY, sizeof(ArenaBlock));
        if (memory == NULL)
        {
            fprintf(stderr, "[%s:%d] Unable to allocate memory.\n", __FILE__, __LINE__);
            exit(1);
        }

        *arena = (Arena)
        {
            .body = memory                               ,
            .capacity = ARENA_MINIMUM_BLOCK_LIST_CAPACITY,
            .size     = 0                                ,
        };
    }
    // Otherwise it is initialized
}

void* arena_push(Arena* arena, void* obj, size_t size)
{
    void* memory = NULL;

    if (arena == NULL)
    {
        fprintf(stderr, "[%s:%d] Cannot allocate memory to a NULL arena.\n", __FILE__, __LINE__);
        exit(1);
    }
    if (obj == NULL)
    {
        fprintf(stderr, "[%s:%d] Cannot push empty object to arena.\n", __FILE__, __LINE__);
        exit(1);
    }
    if (size == 0)
    {
        fprintf(stderr, "[%s:%d] Cannot push objects of size 0 to arena.\n", __FILE__, __LINE__);
        exit(1);
    }

    arena_make_sure_block_list_is_initialized(arena);
    ArenaBlock* block = arena_get_block_with_enough_memory(arena, size);
    // We need to allocate more memory
    if (block == NULL)
    {
        block = arena_allocate_new_block(arena, size);
    }

    memory = arena_block_push(block, obj, size);

    return memory;
}

// Ehh, implementation could be more efficient, but it is what it is.
void* arena_push_empty(Arena* arena, size_t size)
{
    char* mem = NULL;
    void* ret = NULL;

    mem = calloc(size, sizeof(char));
    if (mem == NULL)
    {
        fprintf(stderr, "[%s:%d] Unable to allocate memory.\n", __FILE__, __LINE__);
        exit(1);
    }

    ret = arena_push(arena, mem, size);
    free(mem);
    return ret;
}
