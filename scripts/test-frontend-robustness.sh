#!/usr/bin/env bash
set -euo pipefail

GEE_BIN=${GEE_BIN:-./gee}
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "$WORK_DIR"' EXIT

fail=0

test_parse_recovery() {
  local src="$WORK_DIR/bad_decl.cb"
  cat >"$src" <<'EOF'
extern add3(int64, int64, int64);
fn main() -> int64 { return 0; }
EOF

  local log="$WORK_DIR/bad_decl.log"
  set +e
  "$GEE_BIN" "$src" "$WORK_DIR/bad_decl.s" >"$log" 2>&1
  local rc=$?
  set -e

  if [[ $rc -eq 0 ]]; then
    echo "[FAIL] parse_recovery: expected failure for malformed extern"
    fail=1
    return
  fi

  local lines
  lines=$(wc -l <"$log")
  if [[ $lines -gt 80 ]]; then
    echo "[FAIL] parse_recovery: diagnostics too noisy ($lines lines)"
    fail=1
    return
  fi

  if ! grep -q "Compilation failed due to parse/type errors." "$log"; then
    echo "[FAIL] parse_recovery: missing final compilation failure message"
    fail=1
    return
  fi

  echo "[PASS] parse_recovery"
}

test_conditional_else() {
  local src="$WORK_DIR/ifdef_else.cb"
  cat >"$src" <<'EOF'
import system;
int32 main() {
#ifdef ENABLE_FEATURE
    system_exit(7);
#else
    system_exit(0);
#endif
    }
EOF

  local bin="$WORK_DIR/ifdef_else_bin"
  if ! GEE_BIN="$GEE_BIN" bash "$ROOT_DIR/scripts/gee-asm-link.sh" x86-64 "$src" "$bin" >/dev/null 2>&1; then
    echo "[FAIL] ifdef_else: compile/link failed"
    fail=1
    return
  fi

  if "$bin" >/dev/null 2>&1; then
    echo "[PASS] ifdef_else"
  else
    echo "[FAIL] ifdef_else: runtime exit=$?"
    fail=1
  fi
}

test_import_roots_env() {
  local app_dir="$WORK_DIR/app"
  local sdk_dir="$WORK_DIR/sdk_root"
  mkdir -p "$app_dir" "$sdk_dir/stdlib"

  cat >"$sdk_dir/stdlib/custom.h" <<'EOF'
extern void println(int64);
EOF

  cat >"$app_dir/import_env.cb" <<'EOF'
import custom;
int32 main() {
    return 0;
}
EOF

  if ! GEE_STDLIB="$sdk_dir" "$GEE_BIN" "$app_dir/import_env.cb" "$WORK_DIR/import_env.s" >/dev/null 2>&1; then
    echo "[FAIL] import_env: failed to resolve module via GEE_STDLIB"
    fail=1
    return
  fi

  echo "[PASS] import_env"
}

test_parse_recovery
test_conditional_else
test_import_roots_env

if [[ $fail -ne 0 ]]; then
  exit 1
fi
