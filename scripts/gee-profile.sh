#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "uso: $0 <archivo.cb> [iteraciones]" >&2
  exit 2
fi

SRC="$1"
ITERS="${2:-5}"
GEE_BIN=${GEE_BIN:-./gee}
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

BIN="$TMP_DIR/profile_bin"
GEE_BIN="$GEE_BIN" bash "$ROOT_DIR/scripts/gee-asm-link.sh" x86-64 "$SRC" "$BIN" >/dev/null

echo "profiling: $SRC (iteraciones=$ITERS)"
TOTAL_NS=0
for ((i=1; i<=ITERS; i++)); do
  start=$(date +%s%N)
  "$BIN" >/dev/null 2>&1 || true
  end=$(date +%s%N)
  dur=$((end-start))
  TOTAL_NS=$((TOTAL_NS+dur))
  echo "iter $i: ${dur}ns"
done
avg=$((TOTAL_NS/ITERS))
echo "avg: ${avg}ns"
