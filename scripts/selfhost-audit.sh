#!/usr/bin/env bash
set -euo pipefail

fail=0

echo "[audit] verificando targets self-host/no-cc"
if ! rg -n "^bootstrap-no-cc:" Makefile >/dev/null; then
  echo "[audit][error] falta target bootstrap-no-cc"
  fail=1
fi

if ! rg -n "^advanced: bootstrap-no-cc$" Makefile >/dev/null; then
  echo "[audit][error] advanced no apunta a bootstrap-no-cc"
  fail=1
fi

echo "[audit] verificando módulos selfhost"
for f in selfhost/lexer.cb selfhost/parser.cb selfhost/ast.cb selfhost/codegen.cb selfhost/main.cb; do
  if [[ ! -f "$f" ]]; then
    echo "[audit][error] falta $f"
    fail=1
  fi
done

if [[ $fail -ne 0 ]]; then
  echo "[audit] FAILED"
  exit 1
fi

echo "[audit] OK"
