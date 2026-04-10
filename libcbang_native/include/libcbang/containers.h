#ifndef LIBCBANG_CONTAINERS_H
#define LIBCBANG_CONTAINERS_H

#include <stddef.h>
#include "libcbang/core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cb_vector {
    cb_u8* data;
    size_t len;
    size_t cap;
    size_t elem_size;
} cb_vector;

cb_bool cb_vector_init(cb_vector* v, size_t elem_size);
void cb_vector_free(cb_vector* v);
cb_bool cb_vector_push(cb_vector* v, const void* elem);
void* cb_vector_at(cb_vector* v, size_t index);

typedef struct cb_list_node {
    struct cb_list_node* prev;
    struct cb_list_node* next;
    void* value;
} cb_list_node;

typedef struct cb_list {
    cb_list_node* head;
    cb_list_node* tail;
    size_t len;
} cb_list;

void cb_list_init(cb_list* l);
cb_bool cb_list_push_back(cb_list* l, void* value);
void cb_list_clear(cb_list* l);

typedef struct cb_hash_entry {
    const char* key;
    cb_i64 value;
    cb_bool used;
} cb_hash_entry;

typedef struct cb_hashmap {
    cb_hash_entry* entries;
    size_t cap;
    size_t len;
} cb_hashmap;

cb_bool cb_hashmap_init(cb_hashmap* m, size_t initial_cap);
void cb_hashmap_free(cb_hashmap* m);
cb_bool cb_hashmap_set(cb_hashmap* m, const char* key, cb_i64 value);
cb_bool cb_hashmap_get(const cb_hashmap* m, const char* key, cb_i64* out_value);

#ifdef __cplusplus
}
#endif

#endif
