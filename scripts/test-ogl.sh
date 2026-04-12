#!/usr/bin/env bash
set -euo pipefail

GEE_BIN=${GEE_BIN:-./gee}
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "$WORK_DIR"' EXIT

mods=("$ROOT_DIR/libogl/color.cb" "$ROOT_DIR/libogl/canvas.cb")
mod_asms=()
for m in "${mods[@]}"; do
  out="$WORK_DIR/$(basename "${m%.cb}").s"
  "$GEE_BIN" "$m" "$out" >/dev/null
  mod_asms+=("$out")
done

bin="$WORK_DIR/test_ogl_canvas"
GEE_BIN="$GEE_BIN" bash "$ROOT_DIR/scripts/gee-asm-link.sh" x86-64 "$ROOT_DIR/tests/ogl/test_canvas.cb" "$bin" "${mod_asms[@]}" >/dev/null

set +e
"$bin" >/dev/null 2>&1
rc=$?
set -e

echo "[INFO] test_ogl_canvas expected_exit=0 actual_exit=$rc"
if [[ "$rc" -ne 0 ]]; then
  echo "[FAIL] ogl canvas test"
  exit 1
fi

echo "[PASS] ogl canvas test"
