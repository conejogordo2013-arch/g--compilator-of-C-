#!/usr/bin/env bash
set -euo pipefail

if [ $# -lt 1 ]; then
  echo "Uso: gee-new <nombre_proyecto>"
  exit 1
fi

NAME="$1"
if [ -e "$NAME" ]; then
  echo "Error: '$NAME' ya existe."
  exit 1
fi

mkdir -p "$NAME"

cat > "$NAME/app.h" <<'HDR'
// Header C! del proyecto
extern void println(int64);
extern void print_bool(int32);
extern void print_hex(int64);
HDR

cat > "$NAME/main.cb" <<'SRC'
import io;

extern void println(int64);
extern void print_bool(int32);
extern void print_hex(int64);

int32 main() {
    int64 x = 42;
    println(x);
    print_hex(x);
    print_bool(1);
    return 0;
}
SRC

cat > "$NAME/build.sh" <<'BLD'
#!/usr/bin/env bash
set -euo pipefail

TARGET="${1:-x86-64}"
OUT="main_${TARGET//-/_}.s"
GEE_BIN="${GEE_BIN:-gee}"
if [ "$GEE_BIN" = "gee" ] && ! command -v gee >/dev/null 2>&1; then
  SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
  if [ -x "$SCRIPT_DIR/../gee" ]; then
    GEE_BIN="$SCRIPT_DIR/../gee"
  fi
fi

GEE_TARGET="$TARGET" "$GEE_BIN" main.cb "$OUT"

echo "ASM generado: $OUT"
echo "Para x86-64 (host):"
echo "  cc -no-pie -o main main_x86_64.s ../stdlib/io.s"
echo "Para arm64 (cross):"
echo "  aarch64-linux-gnu-gcc -o main_arm main_arm_64.s ../stdlib/io_arm64.s"
BLD

cat > "$NAME/README.md" <<'DOC'
# Nuevo proyecto C!

Archivos creados:
- `app.h`: declaraciones de runtime
- `main.cb`: entrada inicial
- `build.sh`: helper para emitir asm x86-64/arm-64

## Uso rápido

```bash
./build.sh x86-64
./build.sh arm-64
```
DOC

chmod +x "$NAME/build.sh"

echo "Proyecto C! creado en: $NAME"
