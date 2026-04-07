#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<USAGE
uso: gee-native.sh <programa.cb> [--out binario] [--mode no-cc|host]

Compila para la arquitectura del host automáticamente (sin pasar --target).

Ejemplos:
  bash scripts/gee-native.sh examples/no_cc.cb
  bash scripts/gee-native.sh examples/no_cc.cb --out demo_native
  bash scripts/gee-native.sh examples/no_cc.cb --mode host
USAGE
}

if [[ $# -lt 1 ]]; then
  usage
  exit 2
fi

if [[ "${1:-}" = "-h" || "${1:-}" = "--help" ]]; then
  usage
  exit 0
fi

INPUT="$1"
shift || true

if [[ ! -f "$INPUT" ]]; then
  echo "error: no existe el archivo: $INPUT" >&2
  exit 2
fi

OUT_BIN=""
MODE="no-cc"
while [[ $# -gt 0 ]]; do
  case "$1" in
    --out)
      OUT_BIN="${2:-}"
      shift 2
      ;;
    --mode)
      MODE="${2:-}"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "argumento desconocido: $1" >&2
      usage
      exit 2
      ;;
  esac
done

HOST_ARCH="$(uname -m 2>/dev/null || echo unknown)"
case "$HOST_ARCH" in
  x86_64|amd64)
    TARGET="x86-64"
    ;;
  aarch64|arm64)
    TARGET="arm-64"
    ;;
  *)
    echo "error: arquitectura host no soportada automáticamente: $HOST_ARCH" >&2
    echo "usa scripts/gee-run.sh con --target explícito." >&2
    exit 2
    ;;
esac

if [[ -z "$OUT_BIN" ]]; then
  base="$(basename "${INPUT%.cb}")"
  OUT_BIN="${base}_native"
fi

echo "[gee-native] host=$HOST_ARCH -> target=$TARGET"
echo "[gee-native] mode=$MODE out=$OUT_BIN"

GEE_BIN="${GEE_BIN:-./gee}" bash "$(dirname "$0")/gee-run.sh" "$INPUT" --target "$TARGET" --mode "$MODE" --out "$OUT_BIN"
