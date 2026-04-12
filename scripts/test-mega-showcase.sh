#!/usr/bin/env bash
set -euo pipefail

GEE_BIN=${GEE_BIN:-./gee}
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "$WORK_DIR"' EXIT

src="$ROOT_DIR/examples/mega_showcase.cb"
bin="$WORK_DIR/mega_showcase"
expected=117

lines=$(wc -l < "$src")
echo "[INFO] mega_showcase lines=$lines"

GEE_BIN="$GEE_BIN" bash "$ROOT_DIR/scripts/gee-asm-link.sh" x86-64 "$src" "$bin" >/dev/null
set +e
"$bin" >/dev/null 2>&1
actual=$?
set -e

echo "[INFO] mega_showcase expected_exit=$expected actual_exit=$actual"
if [[ "$actual" -ne "$expected" ]]; then
  echo "[FAIL] mega_showcase mismatch"
  exit 1
fi

echo "[PASS] mega_showcase"
