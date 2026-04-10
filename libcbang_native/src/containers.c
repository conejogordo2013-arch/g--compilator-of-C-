#include "libcbang/containers.h"

#include <string.h>
#include "libcbang/memory.h"

static size_t cb_hash_str(const char* s) {
    size_t h = 1469598103934665603ull;
    while (s && *s) {
        h ^= (unsigned char)*s++;
        h *= 1099511628211ull;
    }
    return h;
}

cb_bool cb_vector_init(cb_vector* v, size_t elem_size) {
    if (!v || elem_size == 0u) return CB_FALSE;
    v->data = NULL; v->len = 0; v->cap = 0; v->elem_size = elem_size;
    return CB_TRUE;
}

void cb_vector_free(cb_vector* v) {
    if (!v) return;
    cb_mem_free(v->data);
    v->data = NULL; v->len = 0; v->cap = 0; v->elem_size = 0;
}

cb_bool cb_vector_push(cb_vector* v, const void* elem) {
    cb_u8* dst;
    size_t next_cap;
    if (!v || !elem) return CB_FALSE;
    if (v->len == v->cap) {
        next_cap = v->cap ? v->cap * 2u : 8u;
        dst = (cb_u8*)cb_mem_realloc(v->data, next_cap * v->elem_size);
        if (!dst) return CB_FALSE;
        v->data = dst;
        v->cap = next_cap;
    }
    dst = v->data + (v->len * v->elem_size);
    memcpy(dst, elem, v->elem_size);
    v->len++;
    return CB_TRUE;
}

void* cb_vector_at(cb_vector* v, size_t index) {
    if (!v || index >= v->len) return NULL;
    return v->data + (index * v->elem_size);
}

void cb_list_init(cb_list* l) {
    if (!l) return;
    l->head = NULL; l->tail = NULL; l->len = 0;
}

cb_bool cb_list_push_back(cb_list* l, void* value) {
    cb_list_node* n;
    if (!l) return CB_FALSE;
    n = (cb_list_node*)cb_mem_alloc(sizeof(cb_list_node));
    if (!n) return CB_FALSE;
    n->prev = l->tail;
    n->next = NULL;
    n->value = value;
    if (l->tail) l->tail->next = n;
    else l->head = n;
    l->tail = n;
    l->len++;
    return CB_TRUE;
}

void cb_list_clear(cb_list* l) {
    cb_list_node* it;
    cb_list_node* next;
    if (!l) return;
    for (it = l->head; it; it = next) {
        next = it->next;
        cb_mem_free(it);
    }
    l->head = NULL; l->tail = NULL; l->len = 0;
}

static cb_bool cb_hashmap_rehash(cb_hashmap* m, size_t new_cap) {
    cb_hash_entry* old = m->entries;
    size_t old_cap = m->cap;
    size_t i;
    m->entries = (cb_hash_entry*)cb_mem_calloc(new_cap, sizeof(cb_hash_entry));
    if (!m->entries) { m->entries = old; m->cap = old_cap; return CB_FALSE; }
    m->cap = new_cap;
    m->len = 0;
    for (i = 0; i < old_cap; ++i) {
        if (old[i].used) cb_hashmap_set(m, old[i].key, old[i].value);
    }
    cb_mem_free(old);
    return CB_TRUE;
}

cb_bool cb_hashmap_init(cb_hashmap* m, size_t initial_cap) {
    if (!m) return CB_FALSE;
    if (initial_cap < 8u) initial_cap = 8u;
    m->entries = (cb_hash_entry*)cb_mem_calloc(initial_cap, sizeof(cb_hash_entry));
    if (!m->entries) return CB_FALSE;
    m->cap = initial_cap;
    m->len = 0;
    return CB_TRUE;
}

void cb_hashmap_free(cb_hashmap* m) {
    if (!m) return;
    cb_mem_free(m->entries);
    m->entries = NULL; m->cap = 0; m->len = 0;
}

cb_bool cb_hashmap_set(cb_hashmap* m, const char* key, cb_i64 value) {
    size_t i;
    size_t idx;
    if (!m || !m->entries || !key) return CB_FALSE;
    if ((m->len + 1u) * 10u > m->cap * 7u) {
        if (!cb_hashmap_rehash(m, m->cap * 2u)) return CB_FALSE;
    }
    idx = cb_hash_str(key) % m->cap;
    for (i = 0; i < m->cap; ++i) {
        cb_hash_entry* e = &m->entries[(idx + i) % m->cap];
        if (!e->used) {
            e->used = CB_TRUE;
            e->key = key;
            e->value = value;
            m->len++;
            return CB_TRUE;
        }
        if (strcmp(e->key, key) == 0) {
            e->value = value;
            return CB_TRUE;
        }
    }
    return CB_FALSE;
}

cb_bool cb_hashmap_get(const cb_hashmap* m, const char* key, cb_i64* out_value) {
    size_t i;
    size_t idx;
    if (!m || !m->entries || !key || !out_value) return CB_FALSE;
    idx = cb_hash_str(key) % m->cap;
    for (i = 0; i < m->cap; ++i) {
        const cb_hash_entry* e = &m->entries[(idx + i) % m->cap];
        if (!e->used) return CB_FALSE;
        if (strcmp(e->key, key) == 0) {
            *out_value = e->value;
            return CB_TRUE;
        }
    }
    return CB_FALSE;
}
