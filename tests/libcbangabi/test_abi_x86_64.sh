#!/usr/bin/env bash
set -euo pipefail

GEE_BIN="${GEE_BIN:-./gee}"
AS_BIN="${AS_BIN:-as}"
LD_BIN="${LD_BIN:-ld}"

bash scripts/build-libcbangabi-shared.sh x86-64 ./libcbangabi.so

"$GEE_BIN" tests/libcbangabi/program_a.cb /tmp/cbabi_program_a.s >/dev/null
"$GEE_BIN" tests/libcbangabi/program_b.cb /tmp/cbabi_program_b.s >/dev/null
"$AS_BIN" --64 -o /tmp/cbabi_program_a.o /tmp/cbabi_program_a.s
"$AS_BIN" --64 -o /tmp/cbabi_program_b.o /tmp/cbabi_program_b.s
"$AS_BIN" --64 -o /tmp/cbabi_system.o stdlib/system.s

"$LD_BIN" -dynamic-linker /lib64/ld-linux-x86-64.so.2 -e main -o /tmp/cbabi_program_bin \
  /tmp/cbabi_program_a.o /tmp/cbabi_program_b.o /tmp/cbabi_system.o -L. -lcbangabi

LD_LIBRARY_PATH=. /tmp/cbabi_program_bin
rc=$?
echo "EXIT:$rc"
exit "$rc"
