#!/usr/bin/env bash
set -euo pipefail

GEE_BIN=${GEE_BIN:-./gee}
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="$(mktemp -d)"
trap 'rm -rf "$OUT_DIR"' EXIT

fail=0
check_compile() {
  local src="$1"
  local out="$OUT_DIR/$(basename "$src").s"
  if "$GEE_BIN" "$src" "$out" >/dev/null 2>&1; then
    echo "[PASS] compile $src"
  else
    echo "[FAIL] compile $src"
    fail=1
  fi
}

for src in "$ROOT_DIR"/examples/*.cb "$ROOT_DIR"/benchmarks/*.cb "$ROOT_DIR"/selfhost/*.cb; do
  [[ -f "$src" ]] || continue
  check_compile "$src"
done

if [[ $fail -ne 0 ]]; then
  exit 1
fi
