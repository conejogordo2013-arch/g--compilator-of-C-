#pragma once

// stutilsio.h
// Utilidades para modelar conceptos tipo typedef/union/void* en C! actual.
// C! no tiene typedef/union/void* nativos 1:1, así que se representan con
// structs y helpers explícitos.

struct st_u8  { byte  v; };
struct st_u16 { int32 v; };
struct st_u32 { int32 v; };
struct st_u64 { int64 v; };
struct st_s32 { int32 v; };
struct st_s64 { int64 v; };

// "union" escalar aproximada en C!
struct st_union_scalar {
    int64 s64;
    int64 u64;
    int64 ptr;
    int32 u32;
    int32 u16;
    byte  u8;
};

int64 ST_NULL = 0;

int64 st_void_from_i64(int64 x) { return x; }
int64 st_i64_from_void(int64 p) { return p; }

int64 st_union_read_s64(struct st_union_scalar* u) {
    return u->s64;
}

void st_union_write_s64(struct st_union_scalar* u, int64 v) {
    u->s64 = v;
    u->u64 = v;
    u->ptr = v;
}
