#ifndef LIBCBANG_RUNTIME_H
#define LIBCBANG_RUNTIME_H

#include "libcbang/core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cb_runtime {
    cb_i32 argc;
    const char** argv;
    cb_bool initialized;
} cb_runtime;

cb_bool cb_runtime_init(cb_runtime* rt, cb_i32 argc, const char** argv);
void cb_runtime_shutdown(cb_runtime* rt);
void cb_runtime_panic(const char* msg);

#ifdef __cplusplus
}
#endif

#endif
