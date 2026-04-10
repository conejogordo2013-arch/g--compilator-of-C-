#ifndef LIBCBANG_ABI_H
#define LIBCBANG_ABI_H

#include "libcbang/core.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CB_ABI_VERSION 0x00010000u

typedef struct cb_abi_header {
    cb_u32 abi_version;
    cb_u32 flags;
    cb_u64 payload;
} cb_abi_header;

cb_u32 cb_abi_version(void);
void cb_abi_header_init(cb_abi_header* h, cb_u64 payload);
cb_bool cb_abi_header_valid(const cb_abi_header* h);

#ifdef __cplusplus
}
#endif

#endif
