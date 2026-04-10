#include <stdio.h>
#include "libcbang/libcbang.h"

int main(int argc, const char** argv) {
    cb_runtime rt;
    cb_string s;
    cb_vector v;
    cb_hashmap m;
    cb_i32 n = 42;
    cb_i64 value = 0;
    cb_abi_header h;

    cb_runtime_init(&rt, argc, argv);

    cb_string_init(&s);
    cb_string_set(&s, "Hello");
    cb_string_append(&s, " C!");
    cb_io_println(s.data);

    cb_vector_init(&v, sizeof(cb_i32));
    cb_vector_push(&v, &n);

    cb_hashmap_init(&m, 8);
    cb_hashmap_set(&m, "answer", 42);
    cb_hashmap_get(&m, "answer", &value);
    printf("hash value=%lld\n", (long long)value);

    cb_abi_header_init(&h, 99);
    if (!cb_abi_header_valid(&h)) return 2;

    cb_hashmap_free(&m);
    cb_vector_free(&v);
    cb_string_free(&s);
    cb_runtime_shutdown(&rt);
    return 0;
}
