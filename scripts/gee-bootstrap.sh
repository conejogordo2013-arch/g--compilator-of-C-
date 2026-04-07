#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

echo "[1/3] Building stage0 (C -> gee)"
make stage0

echo "[2/3] Building stage1 (gee -> selfhost asm -> gee_stage1)"
make bootstrap

echo "[3/3] Smoke run stage1 binary"
if ./gee_stage1; then
  echo "stage1 smoke: OK"
else
  echo "stage1 smoke: FAIL" >&2
  exit 1
fi

echo "Bootstrap pipeline complete."
