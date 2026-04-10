#include "libcbang/string.h"

#include <string.h>
#include "libcbang/memory.h"

static size_t cb_strlen0(const char* s) {
    return s ? strlen(s) : 0u;
}

cb_bool cb_string_init(cb_string* s) {
    if (!s) return CB_FALSE;
    s->data = NULL;
    s->len = 0;
    s->cap = 0;
    return CB_TRUE;
}

void cb_string_free(cb_string* s) {
    if (!s) return;
    cb_mem_free(s->data);
    s->data = NULL;
    s->len = 0;
    s->cap = 0;
}

cb_bool cb_string_reserve(cb_string* s, size_t cap) {
    char* next;
    if (!s) return CB_FALSE;
    if (cap <= s->cap) return CB_TRUE;
    next = (char*)cb_mem_realloc(s->data, cap);
    if (!next) return CB_FALSE;
    s->data = next;
    s->cap = cap;
    return CB_TRUE;
}

cb_bool cb_string_set(cb_string* s, const char* cstr) {
    size_t n = cb_strlen0(cstr);
    if (!cb_string_reserve(s, n + 1u)) return CB_FALSE;
    if (n) memcpy(s->data, cstr, n);
    s->data[n] = '\0';
    s->len = n;
    return CB_TRUE;
}

cb_bool cb_string_append(cb_string* s, const char* cstr) {
    size_t add = cb_strlen0(cstr);
    if (!cb_string_reserve(s, s->len + add + 1u)) return CB_FALSE;
    if (add) memcpy(s->data + s->len, cstr, add);
    s->len += add;
    s->data[s->len] = '\0';
    return CB_TRUE;
}

cb_string cb_string_slice(const cb_string* s, size_t start, size_t len) {
    cb_string out;
    cb_string_init(&out);
    if (!s || !s->data || start >= s->len) return out;
    if (start + len > s->len) len = s->len - start;
    if (!cb_string_reserve(&out, len + 1u)) return out;
    if (len) memcpy(out.data, s->data + start, len);
    out.data[len] = '\0';
    out.len = len;
    return out;
}

cb_i32 cb_string_compare(const cb_string* a, const cb_string* b) {
    if (!a || !b) return 0;
    if (!a->data && !b->data) return 0;
    if (!a->data) return -1;
    if (!b->data) return 1;
    return (cb_i32)strcmp(a->data, b->data);
}
