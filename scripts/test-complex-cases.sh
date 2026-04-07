#!/usr/bin/env bash
set -euo pipefail

GEE_BIN=${GEE_BIN:-./gee}
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "$WORK_DIR"' EXIT

fail=0

run_x86_case() {
  local name="$1"
  local src="$2"
  local expected="$3"
  local stdlib_root="${4:-}"
  local bin="$WORK_DIR/${name}_x86"

  if ! GEE_STDLIB="$stdlib_root" GEE_BIN="$GEE_BIN" bash "$ROOT_DIR/scripts/gee-asm-link.sh" x86-64 "$src" "$bin" >/dev/null 2>&1; then
    echo "[FAIL] $name(x86-64): compile/link failed"
    fail=1
    return
  fi

  set +e
  "$bin" >/dev/null 2>&1
  local rc=$?
  set -e
  if [[ "$rc" -ne "$expected" ]]; then
    echo "[FAIL] $name(x86-64): expected exit=$expected got=$rc"
    fail=1
    return
  fi
  echo "[PASS] $name(x86-64) exit=$rc"
}

check_arm_emit() {
  local name="$1"
  local src="$2"
  local stdlib_root="${3:-}"
  local out="$WORK_DIR/${name}_arm64.s"
  if GEE_STDLIB="$stdlib_root" GEE_TARGET=arm-64 "$GEE_BIN" "$src" "$out" >/dev/null 2>&1; then
    echo "[PASS] $name(arm64): asm emit ok"
  else
    echo "[FAIL] $name(arm64): asm emit failed"
    fail=1
  fi
}

run_host_output_case() {
  local name="$1"
  local src="$2"
  local expected_out="$3"
  local stdlib_root="${4:-}"
  local asm="$WORK_DIR/${name}_host.s"
  local bin="$WORK_DIR/${name}_host_bin"
  local out_txt="$WORK_DIR/${name}_stdout.txt"

  if ! GEE_STDLIB="$stdlib_root" GEE_TARGET=x86-64 "$GEE_BIN" "$src" "$asm" >/dev/null 2>&1; then
    echo "[FAIL] $name(host-output): compile failed"
    fail=1
    return
  fi
  if ! cc -no-pie -o "$bin" "$asm" "$ROOT_DIR/stdlib/io.s" "$ROOT_DIR/stdlib/memory.s" "$ROOT_DIR/stdlib/net.s" "$ROOT_DIR/stdlib/system.s" >/dev/null 2>&1; then
    echo "[FAIL] $name(host-output): host link failed"
    fail=1
    return
  fi
  if ! "$bin" >"$out_txt" 2>/dev/null; then
    echo "[FAIL] $name(host-output): runtime failed"
    fail=1
    return
  fi
  if [[ "$(cat "$out_txt")" != "$expected_out" ]]; then
    echo "[FAIL] $name(host-output): expected output='$expected_out' got='$(cat "$out_txt")'"
    fail=1
    return
  fi
  echo "[PASS] $name(host-output) output='$expected_out'"
}

make_complex_program() {
  local src="$WORK_DIR/complex_alg.cb"
  cat >"$src" <<'EOF'
import system;

struct Pair {
    int32 x;
    int32 y;
};

int32 fib(int32 n) {
    if (n <= 1) return n;
    int32 a = 0;
    int32 b = 1;
    int32 i = 2;
    while (i <= n) {
        int32 c = a + b;
        a = b;
        b = c;
        i = i + 1;
    }
    return b;
}

int32 score(struct Pair* p, int32 n) {
    int32 acc = 0;
    int32 i = 0;
    for (; i < n; i = i + 1) {
        acc = acc + ((i << 1) ^ (p->x + p->y));
        if ((i % 3) == 0) acc = acc + 5;
        if ((i % 4) == 0) acc = acc - 2;
    }

    if (n == 8) acc = acc + 11;
    else if (n == 9) acc = acc + 21;
    else acc = acc + 3;

    return acc + fib(10); // fib(10)=55
}

int32 main() {
    struct Pair p;
    p.x = 4;
    p.y = 9;
    int32 v = score(&p, 9);
    int32 f = fib(10);
    int32 checksum = (v + f) % 64;
    system_exit(checksum);
    return 0;
}
EOF
  printf '%s\n' "$src"
}

make_import_program() {
  local sdk="$WORK_DIR/sdk_root"
  mkdir -p "$sdk/stdlib"

  cat >"$sdk/stdlib/custom_leaf.h" <<'EOF'
#pragma once
extern int32 custom_declared(int32);
EOF

  cat >"$sdk/stdlib/custom_chain.h" <<'EOF'
#pragma once
#include "custom_leaf.h"
extern void println(int64);
EOF

  local src="$WORK_DIR/import_chain.cb"
  cat >"$src" <<'EOF'
import custom_chain;
import system;
int32 main() {
    int32 x = 7;
    if (x != 7) system_exit(71);
    system_exit(23);
    return 0;
}
EOF
  printf '%s\n' "$src"
}

make_output_programs() {
  local src1="$WORK_DIR/output_math.cb"
  cat >"$src1" <<'EOF'
import io;

int32 main() {
    int32 i = 1;
    int32 r = 1;
    while (i < 7) {
        r = r * i;
        i = i + 1;
    }
    int32 v = r + 4;
    println(v);
    return 0;
}
EOF

  local src2="$WORK_DIR/output_struct.cb"
  cat >"$src2" <<'EOF'
import io;

struct Acc {
    int64 a;
    int64 b;
};

int64 fold(struct Acc* s) {
    int64 i = 0;
    int64 sum = 0;
    while (i < 5) {
        sum = sum + s->a + (s->b * i);
        i = i + 1;
    }
    return sum;
}

int32 main() {
    struct Acc s;
    s.a = 3;
    s.b = 2;
    println(fold(&s));
    return 0;
}
EOF

  printf '%s\n%s\n' "$src1" "$src2"
}

complex_src="$(make_complex_program)"
import_src="$(make_import_program)"
mapfile -t output_programs < <(make_output_programs)

run_x86_case "complex_alg" "$complex_src" 41
check_arm_emit "complex_alg" "$complex_src"

if GEE_STDLIB="$WORK_DIR/sdk_root" "$GEE_BIN" "$import_src" "$WORK_DIR/import_chain_x86.s" >/dev/null 2>&1; then
  echo "[PASS] import_chain(resolve): compile ok via GEE_STDLIB"
else
  echo "[FAIL] import_chain(resolve): compile failed via GEE_STDLIB"
  fail=1
fi

run_x86_case "import_chain" "$import_src" 23 "$WORK_DIR/sdk_root"
check_arm_emit "import_chain" "$import_src" "$WORK_DIR/sdk_root"

run_host_output_case "output_math" "${output_programs[0]}" "724"
run_host_output_case "output_struct" "${output_programs[1]}" "35"

if [[ $fail -ne 0 ]]; then
  exit 1
fi
