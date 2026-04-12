#!/usr/bin/env bash
set -euo pipefail

GEE_BIN=${GEE_BIN:-./gee}
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "$WORK_DIR"' EXIT

fail=0

run_case() {
  local name="$1"
  local src="$2"
  local expected="$3"
  shift 3
  local extra=("$@")
  local bin="$WORK_DIR/$name"

  if ! GEE_BIN="$GEE_BIN" bash "$ROOT_DIR/scripts/gee-asm-link.sh" x86-64 "$src" "$bin" "${extra[@]}" >/dev/null; then
    echo "[FAIL] $name compile/link error"
    fail=1
    return
  fi

  set +e
  "$bin" >/dev/null 2>&1
  local actual=$?
  set -e

  echo "[INFO] $name expected_exit=$expected actual_exit=$actual"
  if [[ "$actual" -ne "$expected" ]]; then
    echo "[FAIL] $name mismatch"
    fail=1
  else
    echo "[PASS] $name"
  fi
}

algorithms_asm="$WORK_DIR/algorithms.s"
"$GEE_BIN" "$ROOT_DIR/libcbang/algorithms.cb" "$algorithms_asm" >/dev/null

run_case "prime_workload" "$ROOT_DIR/tests/deep_validation/prime_workload.cb" 13
run_case "state_machine" "$ROOT_DIR/tests/deep_validation/state_machine.cb" 13
run_case "algorithms_integration" "$ROOT_DIR/tests/deep_validation/algorithms_integration.cb" 31 "$algorithms_asm"

arm_out="$WORK_DIR/prime_workload_arm64.s"
if GEE_TARGET=arm-64 "$GEE_BIN" "$ROOT_DIR/tests/deep_validation/prime_workload.cb" "$arm_out" >/dev/null 2>&1; then
  echo "[INFO] prime_workload arm64_asm_lines=$(wc -l < "$arm_out")"
  echo "[PASS] prime_workload arm64 emit"
else
  echo "[FAIL] prime_workload arm64 emit"
  fail=1
fi

if [[ $fail -ne 0 ]]; then
  exit 1
fi
