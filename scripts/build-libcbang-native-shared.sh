#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TARGET="${1:-x86-64}"
OUT_SO="${2:-libcbang.so}"
GEE_BIN="${GEE_BIN:-$ROOT_DIR/gee}"

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

case "$TARGET" in
  x86-64)
    AS_BIN="${AS_BIN:-as}"
    LD_BIN="${LD_BIN:-ld}"
    AS_FLAGS=(--64)
    RT_SYSTEM="$ROOT_DIR/stdlib/system.s"
    RT_MEMORY="$ROOT_DIR/stdlib/memory.s"
    RT_IO="$ROOT_DIR/stdlib/io_nocc.s"
    ;;
  arm-64)
    AS_BIN="${AS_BIN:-aarch64-linux-gnu-as}"
    LD_BIN="${LD_BIN:-aarch64-linux-gnu-ld}"
    AS_FLAGS=()
    RT_SYSTEM="$ROOT_DIR/stdlib/system_arm64.s"
    RT_MEMORY="$ROOT_DIR/stdlib/memory_arm64.s"
    RT_IO="$ROOT_DIR/stdlib/io_nocc_arm64.s"
    ;;
  *)
    echo "uso: $0 <x86-64|arm-64> [out.so]" >&2
    exit 2
    ;;
esac

MODULES=(
  "$ROOT_DIR/libcbang_native/cbang/core.cb"
  "$ROOT_DIR/libcbang_native/cbang/memory.cb"
  "$ROOT_DIR/libcbang_native/cbang/string.cb"
  "$ROOT_DIR/libcbang_native/cbang/io.cb"
  "$ROOT_DIR/libcbang_native/cbang/containers.cb"
  "$ROOT_DIR/libcbang_native/cbang/runtime.cb"
  "$ROOT_DIR/libcbang_native/cbang/abi.cb"
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
"$AS_BIN" "${AS_FLAGS[@]}" -o "$TMP_DIR/io_rt.o" "$RT_IO"
OBJS+=("$TMP_DIR/system.o" "$TMP_DIR/memory_rt.o" "$TMP_DIR/io_rt.o")

"$LD_BIN" -shared -o "$OUT_SO" "${OBJS[@]}"

echo "libcbang_native shared generada: $OUT_SO ($TARGET)"
