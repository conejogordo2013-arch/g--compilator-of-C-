#!/usr/bin/env bash
set -euo pipefail

if [ $# -lt 1 ]; then
  echo "uso: gee-run.sh <programa.cb> [--target x86-64|arm-64] [--mode no-cc|host] [--out binario]" >&2
  exit 2
fi

INPUT="$1"
shift

TARGET="${GEE_TARGET:-x86-64}"
MODE="no-cc"
OUT_BIN=""

while [ $# -gt 0 ]; do
  case "$1" in
    --target)
      TARGET="${2:-}"
      shift 2
      ;;
    --mode)
      MODE="${2:-}"
      shift 2
      ;;
    --out)
      OUT_BIN="${2:-}"
      shift 2
      ;;
    *)
      echo "argumento desconocido: $1" >&2
      exit 2
      ;;
  esac
done

if [ -z "$OUT_BIN" ]; then
  base="$(basename "${INPUT%.cb}")"
  OUT_BIN="${base}_${TARGET//-/_}"
fi

case "$MODE" in
  no-cc)
    GEE_BIN="${GEE_BIN:-./gee}" bash scripts/gee-asm-link.sh "$TARGET" "$INPUT" "$OUT_BIN"
    ;;
  host)
    asm_out="${OUT_BIN}.s"
    GEE_TARGET="$TARGET" "${GEE_BIN:-./gee}" "$INPUT" "$asm_out"
    BUILD_ARCH="$(uname -m 2>/dev/null || echo unknown)"
    if [ "$TARGET" = "x86-64" ] || [ "$TARGET" = "x86_64" ] || [ "$TARGET" = "amd64" ]; then
      "${CC:-cc}" -no-pie -o "$OUT_BIN" "$asm_out" stdlib/io.s stdlib/memory.s stdlib/net.s stdlib/system.s
    elif [ "$TARGET" = "arm-64" ] || [ "$TARGET" = "arm64" ] || [ "$TARGET" = "aarch64" ]; then
      if [ "$BUILD_ARCH" = "aarch64" ] || [ "$BUILD_ARCH" = "arm64" ]; then
        "${CC_ARM64:-${CC:-cc}}" -o "$OUT_BIN" "$asm_out" stdlib/io_arm64.s stdlib/memory_arm64.s stdlib/net_arm64.s stdlib/system_arm64.s
      else
        "${CC_ARM64:-aarch64-linux-gnu-gcc}" -o "$OUT_BIN" "$asm_out" stdlib/io_arm64.s stdlib/memory_arm64.s stdlib/net_arm64.s stdlib/system_arm64.s
      fi
    else
      echo "target no soportado en modo host: $TARGET" >&2
      exit 2
    fi
    ;;
  *)
    echo "modo no soportado: $MODE" >&2
    exit 2
    ;;
esac

echo "binario: $OUT_BIN"

HOST_ARCH="$(uname -m 2>/dev/null || echo unknown)"
RUN_BIN="$OUT_BIN"
if [[ "$RUN_BIN" != /* && "$RUN_BIN" != ./* && "$RUN_BIN" != ../* ]]; then
  RUN_BIN="./$RUN_BIN"
fi

if [ "$TARGET" = "x86-64" ] || [ "$TARGET" = "x86_64" ] || [ "$TARGET" = "amd64" ]; then
  if [ "$HOST_ARCH" = "x86_64" ] || [ "$HOST_ARCH" = "amd64" ]; then
    "$RUN_BIN"
  else
    if command -v qemu-x86_64 >/dev/null 2>&1; then
      qemu-x86_64 "$RUN_BIN"
    else
      echo "aviso: no puedo ejecutar x86-64 en host $HOST_ARCH (falta qemu-x86_64)." >&2
    fi
  fi
else
  if [ "$HOST_ARCH" = "aarch64" ] || [ "$HOST_ARCH" = "arm64" ]; then
    "$RUN_BIN"
  else
    if command -v qemu-aarch64 >/dev/null 2>&1; then
      qemu-aarch64 "$RUN_BIN"
    else
      echo "aviso: no puedo ejecutar arm64 en host $HOST_ARCH (falta qemu-aarch64)." >&2
    fi
  fi
fi
