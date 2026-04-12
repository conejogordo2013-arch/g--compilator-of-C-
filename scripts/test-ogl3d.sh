#!/usr/bin/env bash
set -euo pipefail

GEE_BIN=${GEE_BIN:-./gee}
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "$WORK_DIR"' EXIT

mods=(
  "$ROOT_DIR/libogl3d/vec3.cb"
  "$ROOT_DIR/libogl3d/body3d.cb"
  "$ROOT_DIR/libogl3d/world3d.cb"
  "$ROOT_DIR/libogl3d/debug3d.cb"
)

asms=()
for m in "${mods[@]}"; do
  out="$WORK_DIR/$(basename "${m%.cb}").s"
  "$GEE_BIN" "$m" "$out" >/dev/null
  asms+=("$out")
done

bin="$WORK_DIR/test_ogl3d"
GEE_BIN="$GEE_BIN" bash "$ROOT_DIR/scripts/gee-asm-link.sh" x86-64 "$ROOT_DIR/tests/ogl3d/test_physics3d.cb" "$bin" "${asms[@]}" >/dev/null

set +e
"$bin" >/dev/null 2>&1
rc=$?
set -e

expected=228
echo "[INFO] test_ogl3d expected_exit=$expected actual_exit=$rc"
if [[ "$rc" -ne "$expected" ]]; then
  echo "[FAIL] ogl3d physics"
  exit 1
fi

echo "[PASS] ogl3d physics"
