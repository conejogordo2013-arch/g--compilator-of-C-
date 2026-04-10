#!/usr/bin/env bash
set -euo pipefail

if ! command -v aarch64-linux-gnu-as >/dev/null 2>&1 || ! command -v aarch64-linux-gnu-ld >/dev/null 2>&1; then
  echo "SKIP: toolchain ARM64 no disponible"
  exit 0
fi

GEE_BIN="${GEE_BIN:-./gee}"
AS_BIN="aarch64-linux-gnu-as"
LD_BIN="aarch64-linux-gnu-ld"

bash scripts/build-libcbang-shared.sh arm-64 ./libcbang_shared_arm64.so

GEE_TARGET=arm-64 "$GEE_BIN" examples/libcbang_shared_demo.cb /tmp/libcbang_shared_demo_arm64.s >/dev/null
"$AS_BIN" -o /tmp/libcbang_shared_demo_arm64.o /tmp/libcbang_shared_demo_arm64.s
"$AS_BIN" -o /tmp/libcbang_shared_system_arm64.o stdlib/system_arm64.s

"$LD_BIN" -e main -o /tmp/libcbang_shared_demo_bin_arm64 /tmp/libcbang_shared_demo_arm64.o /tmp/libcbang_shared_system_arm64.o -L. -l:libcbang_shared_arm64.so

if command -v qemu-aarch64 >/dev/null 2>&1; then
  LD_LIBRARY_PATH=. qemu-aarch64 /tmp/libcbang_shared_demo_bin_arm64
  rc=$?
  echo "EXIT:$rc"
  exit "$rc"
fi

echo "SKIP: qemu-aarch64 no disponible (build/link ARM64 OK)"
exit 0
