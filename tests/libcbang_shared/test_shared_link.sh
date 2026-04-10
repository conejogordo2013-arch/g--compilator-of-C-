#!/usr/bin/env bash
set -euo pipefail

GEE_BIN="${GEE_BIN:-./gee}"
AS_BIN="${AS_BIN:-as}"
LD_BIN="${LD_BIN:-ld}"

bash scripts/build-libcbang-shared.sh x86-64 ./libcbang_shared.so

"$GEE_BIN" examples/libcbang_shared_demo.cb /tmp/libcbang_shared_demo.s >/dev/null
"$AS_BIN" --64 -o /tmp/libcbang_shared_demo.o /tmp/libcbang_shared_demo.s
"$AS_BIN" --64 -o /tmp/libcbang_shared_system.o stdlib/system.s

"$LD_BIN" -dynamic-linker /lib64/ld-linux-x86-64.so.2 -e main -o /tmp/libcbang_shared_demo_bin /tmp/libcbang_shared_demo.o /tmp/libcbang_shared_system.o -L. -lcbang_shared

LD_LIBRARY_PATH=. /tmp/libcbang_shared_demo_bin
rc=$?
echo "EXIT:$rc"
exit "$rc"
