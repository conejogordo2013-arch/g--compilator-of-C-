#ifndef LIBCBANG_MEMORY_H
#define LIBCBANG_MEMORY_H

#include <stddef.h>
#include "libcbang/core.h"

#ifdef __cplusplus
extern "C" {
#endif

void* cb_mem_alloc(size_t size);
void* cb_mem_calloc(size_t count, size_t size);
void* cb_mem_realloc(void* ptr, size_t size);
void cb_mem_free(void* ptr);

typedef struct cb_arena {
    cb_u8* data;
    size_t size;
    size_t used;
} cb_arena;

cb_bool cb_arena_init(cb_arena* arena, size_t size);
void cb_arena_reset(cb_arena* arena);
void cb_arena_destroy(cb_arena* arena);
void* cb_arena_alloc(cb_arena* arena, size_t size, size_t align);

#ifdef __cplusplus
}
#endif

#endif
