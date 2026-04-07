#!/usr/bin/env bash
set -euo pipefail

PREFIX_DEFAULT="/data/data/com.termux/files/usr"
if [[ -z "${PREFIX:-}" ]]; then
  SELF_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
  PREFIX="$(cd "$SELF_DIR/.." && pwd)"
else
  PREFIX="${PREFIX}"
fi
if [[ ! -d "$PREFIX/bin" ]]; then
  PREFIX="$PREFIX_DEFAULT"
fi
CFG_DIR="${GEE_CFG_DIR:-$PREFIX/etc/gee}"
CFG_FILE="$CFG_DIR/target"
CORE_BIN="${GEE_CORE_BIN:-$PREFIX/bin/gee-core}"

TARGET="${GEE_TARGET:-}"
if [[ -z "$TARGET" && -f "$CFG_FILE" ]]; then
  TARGET="$(tr -d ' \t\r\n' < "$CFG_FILE")"
fi
if [[ -z "$TARGET" ]]; then
  TARGET="x86-64"
fi

case "$TARGET" in
  x86|x86-64|x86_64|amd64)
    TARGET="x86-64"
    ;;
  arm-64|arm64|aarch64)
    TARGET="arm-64"
    ;;
  arm-v7|armv7)
    echo "error: target '$TARGET' no está soportado todavía." >&2
    echo "valid: x86-64, arm-64" >&2
    exit 2
    ;;
  *)
    echo "error: unknown GEE_TARGET '$TARGET'" >&2
    echo "valid: x86-64, arm-64" >&2
    exit 2
    ;;
esac

if [[ ! -x "$CORE_BIN" ]]; then
  echo "error: missing gee-core at $CORE_BIN" >&2
  echo "run: make install PREFIX=$PREFIX" >&2
  exit 2
fi

exec env GEE_TARGET="$TARGET" "$CORE_BIN" "$@"
