#include "libcbang/core.h"

const char* cb_version_string(void) {
    return "libcbang_native 0.1.0";
}

cb_i32 cb_clamp_i32(cb_i32 value, cb_i32 lo, cb_i32 hi) {
    if (value < lo) return lo;
    if (value > hi) return hi;
    return value;
}
