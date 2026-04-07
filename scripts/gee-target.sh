#!/usr/bin/env bash
set -euo pipefail

PREFIX_DEFAULT="/data/data/com.termux/files/usr"
if [[ -z "${PREFIX:-}" ]]; then
  SELF_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
  PREFIX="$(cd "$SELF_DIR/.." && pwd)"
  [[ -d "$PREFIX/bin" ]] || PREFIX="$PREFIX_DEFAULT"
fi
CFG_DIR="${GEE_CFG_DIR:-$PREFIX/etc/gee}"
CFG_FILE="$CFG_DIR/target"

cmd="${1:-show}"
case "$cmd" in
  show)
    if [[ -f "$CFG_FILE" ]]; then
      cat "$CFG_FILE"
    else
      echo "x86-64"
    fi
    ;;
  list)
    printf "x86\nx86-64\narm-v7\narm-64\n"
    ;;
  set)
    target="${2:-}"
    case "$target" in
      x86|x86-64|arm-v7|arm-64) ;;
      *) echo "uso: gee-target set <x86|x86-64|arm-v7|arm-64>" >&2; exit 2 ;;
    esac
    mkdir -p "$CFG_DIR"
    printf "%s\n" "$target" > "$CFG_FILE"
    echo "target activo: $target"
    ;;
  *)
    echo "uso: gee-target [show|list|set <target>]" >&2
    exit 2
    ;;
esac
