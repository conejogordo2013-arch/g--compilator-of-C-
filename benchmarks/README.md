# Bench vs C (x86 + ARM emit)

This folder provides a tiny apples-to-apples loop benchmark:
- `sum_loop.cb`: C! source compiled by `gee`
- `sum_loop.c`: equivalent C baseline
- `bench_vs_c.sh`: builds/runs both and prints timings

## Run

```bash
benchmarks/bench_vs_c.sh
```

Environment knobs:
- `RUNS` (default `3`)
- `CC` (default `cc`)
- `CFLAGS` (default `-O2`)

## Notes
- The script benchmarks **native x86-64 execution** (where this repo runs by default).
- It also emits **ARM64 assembly** from the same `.cb` file.
- If `aarch64-linux-gnu-gcc` is installed, it will additionally link an ARM64 binary.
