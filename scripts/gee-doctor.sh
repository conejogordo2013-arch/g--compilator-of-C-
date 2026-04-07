#!/usr/bin/env bash
set -euo pipefail

PREFIX_DEFAULT="/data/data/com.termux/files/usr"
if [[ -z "${PREFIX:-}" ]]; then
  SELF_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
  PREFIX="$(cd "$SELF_DIR/.." && pwd)"
  [[ -d "$PREFIX/bin" ]] || PREFIX="$PREFIX_DEFAULT"
fi

BINDIR="${BINDIR:-$PREFIX/bin}"
CFG_FILE="${GEE_CFG_DIR:-$PREFIX/etc/gee}/target"
ACTIVE="x86-64"
[[ -f "$CFG_FILE" ]] && ACTIVE="$(tr -d ' \t\r\n' < "$CFG_FILE")"

HOST_ARCH="$(uname -m 2>/dev/null || echo unknown)"

cat <<INFO
gee doctor
==========
Host arch      : $HOST_ARCH
Active target  : $ACTIVE
gee frontend   : $BINDIR/gee
gee core       : $BINDIR/gee-core
INFO

if [[ -x "$BINDIR/gee-core" ]]; then
  echo "gee-core status : OK"
else
  echo "gee-core status : MISSING"
fi

case "$ACTIVE" in
  x86-64) echo "backend status  : implemented" ;;
  *) echo "backend status  : frontend available, backend pending" ;;
esac
