#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."
make clean >/dev/null
make all >/dev/null
./build/examples/minimal_example >/tmp/libcbang_native_minimal.out
./build/examples/full_example >/tmp/libcbang_native_full.out

echo "EXIT:0"
