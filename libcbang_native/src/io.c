#include "libcbang/io.h"

#include <stdio.h>
#include "libcbang/memory.h"

void cb_io_print(const char* s) {
    fputs(s ? s : "", stdout);
}

void cb_io_println(const char* s) {
    fputs(s ? s : "", stdout);
    fputc('\n', stdout);
}

cb_bool cb_io_read_file(const char* path, char** out_data, size_t* out_size) {
    FILE* f;
    long sz;
    char* buf;
    size_t got;
    if (!path || !out_data || !out_size) return CB_FALSE;
    f = fopen(path, "rb");
    if (!f) return CB_FALSE;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return CB_FALSE; }
    sz = ftell(f);
    if (sz < 0) { fclose(f); return CB_FALSE; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return CB_FALSE; }
    buf = (char*)cb_mem_alloc((size_t)sz + 1u);
    if (!buf) { fclose(f); return CB_FALSE; }
    got = fread(buf, 1u, (size_t)sz, f);
    fclose(f);
    if (got != (size_t)sz) { cb_mem_free(buf); return CB_FALSE; }
    buf[sz] = '\0';
    *out_data = buf;
    *out_size = (size_t)sz;
    return CB_TRUE;
}

cb_bool cb_io_write_file(const char* path, const void* data, size_t size) {
    FILE* f;
    size_t wrote;
    if (!path || (!data && size != 0u)) return CB_FALSE;
    f = fopen(path, "wb");
    if (!f) return CB_FALSE;
    wrote = fwrite(data, 1u, size, f);
    fclose(f);
    return wrote == size ? CB_TRUE : CB_FALSE;
}
