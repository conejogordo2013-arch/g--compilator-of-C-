#!/usr/bin/env bash
set -euo pipefail

TARGET="${1:-x86-64}"
OUT_SO="${2:-libcbang_shared.so}"
GEE_BIN="${GEE_BIN:-./gee}"

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

case "$TARGET" in
  x86-64)
    AS_BIN="${AS_BIN:-as}"
    LD_BIN="${LD_BIN:-ld}"
    AS_FLAGS=(--64)
    RT_SYSTEM="stdlib/system.s"
    RT_MEMORY="stdlib/memory.s"
    RT_IO_NOCC="stdlib/io_nocc.s"
    ;;
  arm-64)
    AS_BIN="${AS_BIN:-aarch64-linux-gnu-as}"
    LD_BIN="${LD_BIN:-aarch64-linux-gnu-ld}"
    AS_FLAGS=()
    RT_SYSTEM="stdlib/system_arm64.s"
    RT_MEMORY="stdlib/memory_arm64.s"
    RT_IO_NOCC="stdlib/io_nocc_arm64.s"
    ;;
  *)
    echo "uso: $0 <x86-64|arm-64> [out.so]" >&2
    exit 2
    ;;
esac

if ! command -v "$AS_BIN" >/dev/null 2>&1; then
  echo "warning: assembler no encontrado para $TARGET: $AS_BIN" >&2
  exit 3
fi
if ! command -v "$LD_BIN" >/dev/null 2>&1; then
  echo "warning: linker no encontrado para $TARGET: $LD_BIN" >&2
  exit 3
fi

MODULES=(
  "libcbang/base.cb"
  "libcbang/memory.cb"
  "libcbang/object.cb"
  "libcbang/oops.cb"
  "libcbang/collections.cb"
  "libcbang/algorithms.cb"
)

OBJS=()
for mod in "${MODULES[@]}"; do
  base="$(basename "${mod%.*}")"
  asm="$TMP_DIR/$base.s"
  obj="$TMP_DIR/$base.o"
  GEE_TARGET="$TARGET" "$GEE_BIN" "$mod" "$asm" >/dev/null
  "$AS_BIN" "${AS_FLAGS[@]}" -o "$obj" "$asm"
  OBJS+=("$obj")
done

"$AS_BIN" "${AS_FLAGS[@]}" -o "$TMP_DIR/system.o" "$RT_SYSTEM"
"$AS_BIN" "${AS_FLAGS[@]}" -o "$TMP_DIR/memory_rt.o" "$RT_MEMORY"
"$AS_BIN" "${AS_FLAGS[@]}" -o "$TMP_DIR/io_rt.o" "$RT_IO_NOCC"
OBJS+=("$TMP_DIR/system.o" "$TMP_DIR/memory_rt.o" "$TMP_DIR/io_rt.o")

"$LD_BIN" -shared -o "$OUT_SO" "${OBJS[@]}"

echo "libcbang shared generada: $OUT_SO ($TARGET)"
