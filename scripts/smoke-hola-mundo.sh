#!/usr/bin/env bash
set -euo pipefail

GEE_BIN=${GEE_BIN:-./gee}
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "$WORK_DIR"' EXIT

SRC="$WORK_DIR/hola_mundo.cb"
ASM="$WORK_DIR/hola_mundo.s"
BIN="$WORK_DIR/hola_mundo_bin"
OUT="$WORK_DIR/hola_mundo.out"

cat >"$SRC" <<'EOF'
extern int32 puts(string);

int32 main() {
    puts("Hola Mundo");
    return 0;
}
EOF

"$GEE_BIN" "$SRC" "$ASM" >/dev/null
cc -no-pie -o "$BIN" "$ASM" "$ROOT_DIR/stdlib/io.s" "$ROOT_DIR/stdlib/memory.s" "$ROOT_DIR/stdlib/net.s" "$ROOT_DIR/stdlib/system.s" >/dev/null 2>&1
"$BIN" >"$OUT"

actual="$(tr -d '\r' < "$OUT" | sed 's/[[:space:]]*$//')"
if [[ "$actual" != "Hola Mundo" ]]; then
  echo "[FAIL] hola_mundo: expected='Hola Mundo' got='$actual'"
  exit 1
fi

echo "[PASS] hola_mundo: $actual"

