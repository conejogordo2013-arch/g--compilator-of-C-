#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

make clean >/dev/null
make all >/dev/null

test -f build/lib/libcbang.so

if command -v file >/dev/null 2>&1; then
  file build/lib/libcbang.so | grep -E "shared object|ELF" >/dev/null
fi

echo "EXIT:0"
