#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

CC_BIN="${CC:-cc}"
CFLAGS="${CFLAGS:--O2}"
RUNS="${RUNS:-3}"

mkdir -p benchmarks/out

make stage0 >/dev/null

# Build C! -> x86_64 asm -> native binary
GEE_TARGET=x86-64 ./gee benchmarks/sum_loop.cb benchmarks/out/sum_loop_gee_x86.s >/dev/null
$CC_BIN -O2 -no-pie -o benchmarks/out/sum_loop_gee_x86 benchmarks/out/sum_loop_gee_x86.s stdlib/io.s

# Build C baseline
$CC_BIN $CFLAGS -o benchmarks/out/sum_loop_c benchmarks/sum_loop.c

echo "== Correctness check =="
./benchmarks/out/sum_loop_gee_x86 | tail -n 1
./benchmarks/out/sum_loop_c | tail -n 1

echo
echo "== Timing (lower is better) =="
for i in $(seq 1 "$RUNS"); do
  echo "Run $i:"
  if command -v /usr/bin/time >/dev/null 2>&1; then
    /usr/bin/time -f "  GEE x86: %e s" ./benchmarks/out/sum_loop_gee_x86 >/dev/null
    /usr/bin/time -f "  C  O2 : %e s" ./benchmarks/out/sum_loop_c >/dev/null
  else
    TIMEFORMAT="  GEE x86: %R s"; time ./benchmarks/out/sum_loop_gee_x86 >/dev/null
    TIMEFORMAT="  C  O2 : %R s"; time ./benchmarks/out/sum_loop_c >/dev/null
  fi
  echo
done

# Optional ARM64 generation and cross-compile if toolchain exists
GEE_TARGET=arm-64 ./gee benchmarks/sum_loop.cb benchmarks/out/sum_loop_gee_arm64.s >/dev/null
if command -v aarch64-linux-gnu-gcc >/dev/null 2>&1; then
  aarch64-linux-gnu-gcc -O2 -o benchmarks/out/sum_loop_gee_arm64 benchmarks/out/sum_loop_gee_arm64.s
  echo "ARM64 binary built: benchmarks/out/sum_loop_gee_arm64"
else
  echo "ARM64 asm generated at benchmarks/out/sum_loop_gee_arm64.s (cross-compiler not found, binary not linked)."
fi
