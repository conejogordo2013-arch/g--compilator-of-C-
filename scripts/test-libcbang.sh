#!/usr/bin/env bash
set -euo pipefail

GEE_BIN=${GEE_BIN:-./gee}
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="$(mktemp -d)"
trap 'rm -rf "$OUT_DIR"' EXIT

algorithms_asm="$OUT_DIR/algorithms.s"
"$GEE_BIN" "$ROOT_DIR/libcbang/algorithms.cb" "$algorithms_asm" >/dev/null

src="$ROOT_DIR/tests/libcbang/test_algorithms.cb"
bin="$OUT_DIR/test_algorithms"

fail=0
if ! GEE_BIN="$GEE_BIN" bash "$ROOT_DIR/scripts/gee-asm-link.sh" x86-64 "$src" "$bin" "$algorithms_asm" >/dev/null; then
  echo "[FAIL] compile: test_algorithms"
  fail=1
elif "$bin" >/dev/null 2>&1; then
  echo "[PASS] test_algorithms"
else
  echo "[FAIL] runtime: test_algorithms (exit=$?)"
  fail=1
fi

if [[ $fail -ne 0 ]]; then
  exit 1
fi
