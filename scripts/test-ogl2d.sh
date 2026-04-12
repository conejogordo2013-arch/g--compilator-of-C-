#!/usr/bin/env bash
set -euo pipefail

GEE_BIN=${GEE_BIN:-./gee}
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "$WORK_DIR"' EXIT

mods=(
  "$ROOT_DIR/libogl2d/vec2.cb"
  "$ROOT_DIR/libogl2d/body2d.cb"
  "$ROOT_DIR/libogl2d/world2d.cb"
  "$ROOT_DIR/libogl2d/debug2d.cb"
)

asms=()
for m in "${mods[@]}"; do
  out="$WORK_DIR/$(basename "${m%.cb}").s"
  "$GEE_BIN" "$m" "$out" >/dev/null
  asms+=("$out")
done

bin="$WORK_DIR/test_ogl2d"
GEE_BIN="$GEE_BIN" bash "$ROOT_DIR/scripts/gee-asm-link.sh" x86-64 "$ROOT_DIR/tests/ogl2d/test_physics2d.cb" "$bin" "${asms[@]}" >/dev/null

set +e
"$bin" >/dev/null 2>&1
rc=$?
set -e

expected=148
echo "[INFO] test_ogl2d expected_exit=$expected actual_exit=$rc"
if [[ "$rc" -ne "$expected" ]]; then
  echo "[FAIL] ogl2d physics"
  exit 1
fi

echo "[PASS] ogl2d physics"
