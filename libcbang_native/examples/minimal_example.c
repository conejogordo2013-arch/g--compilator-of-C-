#include <stdio.h>
#include "libcbang/core.h"
#include "libcbang/memory.h"

int main(void) {
    cb_arena arena;
    if (!cb_arena_init(&arena, 128)) return 1;
    if (!cb_arena_alloc(&arena, 16, 8)) return 2;
    printf("%s\n", cb_version_string());
    cb_arena_destroy(&arena);
    return 0;
}
