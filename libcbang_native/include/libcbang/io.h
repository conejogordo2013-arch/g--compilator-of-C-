#ifndef LIBCBANG_IO_H
#define LIBCBANG_IO_H

#include <stddef.h>
#include "libcbang/core.h"

#ifdef __cplusplus
extern "C" {
#endif

void cb_io_print(const char* s);
void cb_io_println(const char* s);
cb_bool cb_io_read_file(const char* path, char** out_data, size_t* out_size);
cb_bool cb_io_write_file(const char* path, const void* data, size_t size);

#ifdef __cplusplus
}
#endif

#endif
