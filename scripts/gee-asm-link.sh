#!/usr/bin/env bash
set -euo pipefail

if [ $# -lt 2 ]; then
  echo "uso: gee-asm-link.sh <target:x86-64|arm-64> <input.cb> [output_bin] [extra_asm.s ...]" >&2
  exit 2
fi

TARGET="$1"
INPUT="$2"
OUT_BIN="${3:-a.out}"
shift 3 || true
EXTRA_ASM=("$@")
BASE="${OUT_BIN%.*}"
ASM_OUT="${BASE}_${TARGET//-/_}.s"
OBJ_OUT="${BASE}_${TARGET//-/_}.o"

GEE_BIN="${GEE_BIN:-./gee}"
HOST_ARCH="$(uname -m 2>/dev/null || echo unknown)"

reject_c_driver() {
  local tool_basename
  tool_basename="$(basename "$1")"
  case "$tool_basename" in
    cc|gcc|clang|clang++|g++|c++)
      echo "error: herramienta no permitida en flujo no-cc: $1" >&2
      echo "usa assembler/linker reales (as/ld o variantes cross)." >&2
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
    STDLIB_SYSTEM="stdlib/system.s"
    STDLIB_MEMORY="stdlib/memory.s"
    STDLIB_NET="stdlib/net.s"
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
    STDLIB_SYSTEM="stdlib/system_nocc_arm64.s"
    STDLIB_MEMORY="stdlib/memory_nocc_arm64.s"
    STDLIB_NET="stdlib/net_arm64.s"
    ;;
  *)
    echo "target no soportado: $TARGET" >&2
    exit 2
    ;;
esac

if ! command -v "$AS_BIN" >/dev/null 2>&1; then
  echo "error: assembler '$AS_BIN' no encontrado." >&2
  echo "tip: en Termux instala 'binutils' (pkg install binutils)." >&2
  exit 2
fi
if ! command -v "$LD_BIN" >/dev/null 2>&1; then
  echo "error: linker '$LD_BIN' no encontrado." >&2
  echo "tip: en Termux instala 'binutils' (pkg install binutils)." >&2
  exit 2
fi

reject_c_driver "$AS_BIN"
reject_c_driver "$LD_BIN"

GEE_TARGET="$TARGET" "$GEE_BIN" "$INPUT" "$ASM_OUT"

"$AS_BIN" "${AS_FLAGS[@]}" -o "$OBJ_OUT" "$ASM_OUT"
"$AS_BIN" "${AS_FLAGS[@]}" -o "${BASE}_system.o" "$STDLIB_SYSTEM"
"$AS_BIN" "${AS_FLAGS[@]}" -o "${BASE}_memory.o" "$STDLIB_MEMORY"
"$AS_BIN" "${AS_FLAGS[@]}" -o "${BASE}_net.o" "$STDLIB_NET"

EXTRA_OBJS=()
for extra in "${EXTRA_ASM[@]}"; do
  obj="${BASE}_$(basename "${extra%.*}").o"
  "$AS_BIN" "${AS_FLAGS[@]}" -o "$obj" "$extra"
  EXTRA_OBJS+=("$obj")
done

"$LD_BIN" -e main -o "$OUT_BIN" "$OBJ_OUT" "${BASE}_system.o" "${BASE}_memory.o" "${BASE}_net.o" "${EXTRA_OBJS[@]}"

echo "binario generado sin cc/clang/gcc: $OUT_BIN"
echo "pipeline usado: GEE (IR->ASM) + as (ASM->OBJ) + ld (OBJ->BIN)"
