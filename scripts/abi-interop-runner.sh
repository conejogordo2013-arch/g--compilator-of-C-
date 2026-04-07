#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GEE_BIN=${GEE_BIN:-./gee}
OUT_DIR="$(mktemp -d)"
trap 'rm -rf "$OUT_DIR"' EXIT

fail=0

echo "[abi] x86-64 interop"
X86_BIN="$OUT_DIR/abi_x86"
if GEE_BIN="$GEE_BIN" bash "$ROOT_DIR/scripts/gee-asm-link.sh" x86-64 "$ROOT_DIR/tests/abi/interop_add3.cb" "$X86_BIN" "$ROOT_DIR/tests/abi/abi_add3_x86_64.s" >/dev/null; then
  if "$X86_BIN" >/dev/null 2>&1; then
    echo "[PASS] x86-64 abi interop"
  else
    echo "[FAIL] x86-64 runtime abi interop"
    fail=1
  fi
else
  echo "[FAIL] x86-64 build abi interop"
  fail=1
fi

echo "[abi] arm64 interop"
if command -v aarch64-linux-gnu-as >/dev/null 2>&1 && command -v aarch64-linux-gnu-ld >/dev/null 2>&1; then
  ARM_ASM="$OUT_DIR/interop_arm64.s"
  "$GEE_BIN" "$ROOT_DIR/tests/abi/interop_add3.cb" "$ARM_ASM" arm-64 >/dev/null
  aarch64-linux-gnu-as -o "$OUT_DIR/interop_arm64.o" "$ARM_ASM"
  aarch64-linux-gnu-as -o "$OUT_DIR/abi_add3_arm64.o" "$ROOT_DIR/tests/abi/abi_add3_arm64.s"
  aarch64-linux-gnu-as -o "$OUT_DIR/system_arm64.o" "$ROOT_DIR/stdlib/system_arm64.s"
  aarch64-linux-gnu-ld -o "$OUT_DIR/abi_arm64" "$OUT_DIR/interop_arm64.o" "$OUT_DIR/abi_add3_arm64.o" "$OUT_DIR/system_arm64.o"
  if command -v qemu-aarch64 >/dev/null 2>&1; then
    if qemu-aarch64 "$OUT_DIR/abi_arm64" >/dev/null 2>&1; then
      echo "[PASS] arm64 abi interop"
    else
      echo "[FAIL] arm64 runtime abi interop"
      fail=1
    fi
  else
    echo "[WARN] qemu-aarch64 no disponible; arm64 compilado pero no ejecutado"
  fi
else
  echo "[WARN] toolchain arm64 no disponible; arm64 omitido"
fi

if [[ $fail -ne 0 ]]; then
  exit 1
fi
