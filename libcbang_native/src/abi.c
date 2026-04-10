#include "libcbang/abi.h"

cb_u32 cb_abi_version(void) {
    return CB_ABI_VERSION;
}

void cb_abi_header_init(cb_abi_header* h, cb_u64 payload) {
    if (!h) return;
    h->abi_version = CB_ABI_VERSION;
    h->flags = 0u;
    h->payload = payload;
}

cb_bool cb_abi_header_valid(const cb_abi_header* h) {
    if (!h) return CB_FALSE;
    return h->abi_version == CB_ABI_VERSION ? CB_TRUE : CB_FALSE;
}
