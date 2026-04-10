#include "libcbang/memory.h"

#include <stdlib.h>

void* cb_mem_alloc(size_t size) { return malloc(size); }
void* cb_mem_calloc(size_t count, size_t size) { return calloc(count, size); }
void* cb_mem_realloc(void* ptr, size_t size) { return realloc(ptr, size); }
void cb_mem_free(void* ptr) { free(ptr); }

cb_bool cb_arena_init(cb_arena* arena, size_t size) {
    if (!arena || size == 0) return CB_FALSE;
    arena->data = (cb_u8*)cb_mem_alloc(size);
    if (!arena->data) return CB_FALSE;
    arena->size = size;
    arena->used = 0;
    return CB_TRUE;
}

void cb_arena_reset(cb_arena* arena) {
    if (arena) arena->used = 0;
}

void cb_arena_destroy(cb_arena* arena) {
    if (!arena) return;
    cb_mem_free(arena->data);
    arena->data = NULL;
    arena->size = 0;
    arena->used = 0;
}

void* cb_arena_alloc(cb_arena* arena, size_t size, size_t align) {
    size_t at;
    if (!arena || !arena->data || size == 0) return NULL;
    if (align == 0) align = sizeof(void*);
    at = (arena->used + (align - 1u)) & ~(align - 1u);
    if (at + size > arena->size) return NULL;
    arena->used = at + size;
    return arena->data + at;
}
