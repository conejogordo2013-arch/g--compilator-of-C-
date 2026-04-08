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

  if ! GEE_PATH="  $sdk_dir  " "$GEE_BIN" "$app_dir/import_env.cb" "$WORK_DIR/import_env_path.s" >/dev/null 2>&1; then
    echo "[FAIL] import_env: failed to resolve module via whitespace-padded GEE_PATH"
    fail=1
    return
  fi

  echo "[PASS] import_env"
}

test_import_local_header() {
  local app_dir="$WORK_DIR/local_import"
  mkdir -p "$app_dir"

  cat >"$app_dir/localmod.h" <<'EOF'
extern int32 puts(string);
EOF

  cat >"$app_dir/main.cb" <<'EOF'
import localmod;
int32 main() {
    puts("local-import-ok");
    return 0;
}
EOF

  if ! "$GEE_BIN" "$app_dir/main.cb" "$WORK_DIR/local_import.s" >/dev/null 2>&1; then
    echo "[FAIL] import_local_header: failed to resolve local <module>.h import"
    fail=1
    return
  fi

  echo "[PASS] import_local_header"
}

test_preprocessor_balance() {
  local src="$WORK_DIR/pre_bad.cb"
  cat >"$src" <<'EOF'
int32 main() {
#ifdef FEATURE_A
    return 0;
}
EOF

  local log="$WORK_DIR/pre_bad.log"
  set +e
  "$GEE_BIN" "$src" "$WORK_DIR/pre_bad.s" >"$log" 2>&1
  local rc=$?
  set -e

  if [[ $rc -eq 0 ]]; then
    echo "[FAIL] preprocessor_balance: expected failure for unterminated #ifdef"
    fail=1
    return
  fi

  if ! grep -q "unterminated preprocessor conditional" "$log"; then
    echo "[FAIL] preprocessor_balance: missing unterminated-conditional diagnostic"
    fail=1
    return
  fi

  echo "[PASS] preprocessor_balance"
}

test_preprocessor_duplicate_else() {
  local src="$WORK_DIR/pre_dup_else.cb"
  cat >"$src" <<'EOF'
int32 main() {
#ifdef FEATURE_A
    return 1;
#else
    return 0;
#else
    return 2;
#endif
}
EOF

  local log="$WORK_DIR/pre_dup_else.log"
  set +e
  "$GEE_BIN" "$src" "$WORK_DIR/pre_dup_else.s" >"$log" 2>&1
  local rc=$?
  set -e

  if [[ $rc -eq 0 ]]; then
    echo "[FAIL] preprocessor_duplicate_else: expected failure for duplicate #else"
    fail=1
    return
  fi

  if ! grep -q "duplicate #else" "$log"; then
    echo "[FAIL] preprocessor_duplicate_else: missing duplicate-else diagnostic"
    fail=1
    return
  fi

  echo "[PASS] preprocessor_duplicate_else"
}

test_invalid_hex_literal() {
  local src="$WORK_DIR/bad_hex.cb"
  cat >"$src" <<'EOF'
int32 main() {
    int32 x = 0x;
    return x;
}
EOF

  local log="$WORK_DIR/bad_hex.log"
  set +e
  "$GEE_BIN" "$src" "$WORK_DIR/bad_hex.s" >"$log" 2>&1
  local rc=$?
  set -e

  if [[ $rc -eq 0 ]]; then
    echo "[FAIL] invalid_hex_literal: expected failure for 0x without digits"
    fail=1
    return
  fi

  if ! grep -q "invalid hexadecimal literal" "$log"; then
    echo "[FAIL] invalid_hex_literal: missing invalid-hex diagnostic"
    fail=1
    return
  fi

  echo "[PASS] invalid_hex_literal"
}

test_preprocessor_elif_not_supported() {
  local src="$WORK_DIR/pre_elif.cb"
  cat >"$src" <<'EOF'
int32 main() {
#ifdef FEATURE_A
    return 1;
#elif FEATURE_B
    return 2;
#else
    return 0;
#endif
}
EOF

  local log="$WORK_DIR/pre_elif.log"
  set +e
  "$GEE_BIN" "$src" "$WORK_DIR/pre_elif.s" >"$log" 2>&1
  local rc=$?
  set -e

  if [[ $rc -eq 0 ]]; then
    echo "[FAIL] preprocessor_elif_not_supported: expected failure for #elif"
    fail=1
    return
  fi

  if ! grep -q "#elif is not supported yet" "$log"; then
    echo "[FAIL] preprocessor_elif_not_supported: missing #elif diagnostic"
    fail=1
    return
  fi

  echo "[PASS] preprocessor_elif_not_supported"
}

test_parse_recovery
test_conditional_else
test_import_roots_env
test_import_local_header
test_preprocessor_balance
test_preprocessor_duplicate_else
test_invalid_hex_literal
test_preprocessor_elif_not_supported

if [[ $fail -ne 0 ]]; then
  exit 1
fi
