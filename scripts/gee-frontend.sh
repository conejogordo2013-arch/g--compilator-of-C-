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
  x86-64|x86_64)
    if [[ ! -x "$CORE_BIN" ]]; then
      echo "error: missing gee-core at $CORE_BIN" >&2
      echo "run: make install PREFIX=$PREFIX" >&2
      exit 2
    fi
    exec "$CORE_BIN" "$@"
    ;;
  x86|arm-v7|arm-64|arm64|armv7)
    echo "error: target '$TARGET' frontend installed, but backend is not implemented yet." >&2
    echo "current backend available: x86-64." >&2
    echo "use: GEE_TARGET=x86-64 gee <input.cb> [output.s]" >&2
    exit 3
    ;;
  *)
    echo "error: unknown GEE_TARGET '$TARGET'" >&2
    echo "valid: x86, x86-64, arm-v7, arm-64" >&2
    exit 2
    ;;
esac
