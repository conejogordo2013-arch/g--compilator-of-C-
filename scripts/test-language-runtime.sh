#!/usr/bin/env bash
set -euo pipefail

GEE_BIN=${GEE_BIN:-./gee}
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TEST_DIR="$ROOT_DIR/tests/language_runtime"
OUT_DIR="$(mktemp -d)"
trap 'rm -rf "$OUT_DIR"' EXIT

fail=0
for src in "$TEST_DIR"/*.cb; do
  name="$(basename "$src" .cb)"
  bin="$OUT_DIR/$name"
  if ! GEE_BIN="$GEE_BIN" bash "$ROOT_DIR/scripts/gee-asm-link.sh" x86-64 "$src" "$bin" >/dev/null; then
    echo "[FAIL] compile: $name"
    fail=1
    continue
  fi
  if "$bin" >/dev/null 2>&1; then
    echo "[PASS] $name"
  else
    echo "[FAIL] runtime: $name (exit=$?)"
    fail=1
  fi
done

if [[ $fail -ne 0 ]]; then
  exit 1
fi
