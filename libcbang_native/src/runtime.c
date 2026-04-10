#include "libcbang/runtime.h"

#include <stdio.h>
#include <stdlib.h>

cb_bool cb_runtime_init(cb_runtime* rt, cb_i32 argc, const char** argv) {
    if (!rt) return CB_FALSE;
    rt->argc = argc;
    rt->argv = argv;
    rt->initialized = CB_TRUE;
    return CB_TRUE;
}

void cb_runtime_shutdown(cb_runtime* rt) {
    if (!rt) return;
    rt->initialized = CB_FALSE;
}

void cb_runtime_panic(const char* msg) {
    fprintf(stderr, "[libcbang panic] %s\n", msg ? msg : "(null)");
    abort();
}
