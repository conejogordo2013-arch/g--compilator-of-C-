#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

TARGET="${1:-x86-64}"
OUT_BIN="${2:-gee_stage1_nocc}"
BUILD_DIR="${BUILD_DIR:-.gee_stage1_nocc}"
GEE_BIN="${GEE_BIN:-./gee}"
HOST_ARCH="$(uname -m 2>/dev/null || echo unknown)"

reject_c_driver() {
  local tool_basename
  tool_basename="$(basename "$1")"
  case "$tool_basename" in
    cc|gcc|clang|clang++|g++|c++)
      echo "error: herramienta no permitida en flujo no-cc: $1" >&2
      exit 2
      ;;
  esac
}

case "$TARGET" in
  x86-64|x86_64|amd64)
    TARGET="x86-64"
    AS_BIN="${AS_BIN:-as}"
    LD_BIN="${LD_BIN:-ld}"
    AS_FLAGS=(--64)
    STDLIB_FILES=(stdlib/io_nocc.s stdlib/memory.s stdlib/net.s stdlib/system.s)
    ;;
  arm-64|arm64|aarch64)
    TARGET="arm-64"
    if [ "$HOST_ARCH" = "aarch64" ] || [ "$HOST_ARCH" = "arm64" ]; then
      AS_BIN="${AS_BIN:-as}"
      LD_BIN="${LD_BIN:-ld}"
    else
      AS_BIN="${AS_BIN:-aarch64-linux-gnu-as}"
      LD_BIN="${LD_BIN:-aarch64-linux-gnu-ld}"
    fi
    AS_FLAGS=()
    STDLIB_FILES=(stdlib/io_nocc_arm64.s stdlib/memory_nocc_arm64.s stdlib/net_arm64.s stdlib/system_nocc_arm64.s)
    ;;
  *)
    echo "target no soportado: $TARGET" >&2
    exit 2
    ;;
esac

if ! [ -x "$GEE_BIN" ]; then
  echo "error: gee no encontrado en '$GEE_BIN'." >&2
  exit 2
fi

for tool in "$AS_BIN" "$LD_BIN"; do
  if ! command -v "$tool" >/dev/null 2>&1; then
    echo "error: herramienta '$tool' no encontrada." >&2
    exit 2
  fi
  reject_c_driver "$tool"
done

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

SELFHOST_SRCS=(selfhost/lexer.cb selfhost/parser.cb selfhost/ast.cb selfhost/codegen.cb selfhost/main.cb)
ALL_OBJS=()

for src in "${SELFHOST_SRCS[@]}"; do
  base="$(basename "${src%.cb}")"
  asm="$BUILD_DIR/$base.s"
  obj="$BUILD_DIR/$base.o"
  GEE_TARGET="$TARGET" "$GEE_BIN" "$src" "$asm"
  "$AS_BIN" "${AS_FLAGS[@]}" -o "$obj" "$asm"
  ALL_OBJS+=("$obj")
done

for stdlib_src in "${STDLIB_FILES[@]}"; do
  base="$(basename "${stdlib_src%.s}")"
  obj="$BUILD_DIR/$base.o"
  "$AS_BIN" "${AS_FLAGS[@]}" -o "$obj" "$stdlib_src"
  ALL_OBJS+=("$obj")
done

"$LD_BIN" -e main -o "$OUT_BIN" "${ALL_OBJS[@]}"
echo "stage1 generado sin cc/clang/gcc: $OUT_BIN"
