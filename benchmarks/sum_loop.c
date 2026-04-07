#include <stdint.h>
#include <stdio.h>

static int64_t bench_sum(int64_t n) {
    int64_t i = 0;
    int64_t s = 0;
    while (i < n) {
        s = s + (i * 3) + 1;
        i = i + 1;
    }
    return s;
}

int main(void) {
    int64_t n = 50000000;
    int64_t r = bench_sum(n);
    printf("%lld\n", (long long)r);
    return 0;
}
