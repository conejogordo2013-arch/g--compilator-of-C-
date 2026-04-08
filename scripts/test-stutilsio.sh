#!/usr/bin/env bash
set -euo pipefail

GEE_BIN=${GEE_BIN:-./gee}
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "$WORK_DIR"' EXIT

ASM="$WORK_DIR/stutilsio_demo.s"
BIN="$WORK_DIR/stutilsio_demo_bin"
OUT="$WORK_DIR/stutilsio_demo.out"

"$GEE_BIN" "$ROOT_DIR/examples/stutilsio_demo.cb" "$ASM" >/dev/null
cc -no-pie -o "$BIN" "$ASM" "$ROOT_DIR/stdlib/io.s" "$ROOT_DIR/stdlib/memory.s" "$ROOT_DIR/stdlib/net.s" "$ROOT_DIR/stdlib/system.s" >/dev/null 2>&1
"$BIN" > "$OUT"

expected=$'1234\n1300'
actual="$(tr -d '\r' < "$OUT" | sed 's/[[:space:]]*$//')"

if [[ "$actual" != "$expected" ]]; then
  echo "[FAIL] stutilsio_demo output mismatch"
  echo "expected:"
  printf "%s\n" "$expected"
  echo "actual:"
  printf "%s\n" "$actual"
  exit 1
fi

echo "[PASS] stutilsio_demo output ok"
