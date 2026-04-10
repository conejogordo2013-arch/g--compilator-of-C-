#ifndef LIBCBANG_CORE_H
#define LIBCBANG_CORE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int8_t cb_i8;
typedef int16_t cb_i16;
typedef int32_t cb_i32;
typedef int64_t cb_i64;
typedef uint8_t cb_u8;
typedef uint16_t cb_u16;
typedef uint32_t cb_u32;
typedef uint64_t cb_u64;
typedef cb_i32 cb_bool;

enum { CB_FALSE = 0, CB_TRUE = 1 };

enum {
    CB_VERSION_MAJOR = 0,
    CB_VERSION_MINOR = 1,
    CB_VERSION_PATCH = 0
};

#define CB_UNUSED(x) ((void)(x))
#define CB_ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

const char* cb_version_string(void);
cb_i32 cb_clamp_i32(cb_i32 value, cb_i32 lo, cb_i32 hi);

#ifdef __cplusplus
}
#endif

#endif
