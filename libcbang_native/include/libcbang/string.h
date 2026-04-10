#ifndef LIBCBANG_STRING_H
#define LIBCBANG_STRING_H

#include <stddef.h>
#include "libcbang/core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cb_string {
    char* data;
    size_t len;
    size_t cap;
} cb_string;

cb_bool cb_string_init(cb_string* s);
void cb_string_free(cb_string* s);
cb_bool cb_string_reserve(cb_string* s, size_t cap);
cb_bool cb_string_set(cb_string* s, const char* cstr);
cb_bool cb_string_append(cb_string* s, const char* cstr);
cb_string cb_string_slice(const cb_string* s, size_t start, size_t len);
cb_i32 cb_string_compare(const cb_string* a, const cb_string* b);

#ifdef __cplusplus
}
#endif

#endif
