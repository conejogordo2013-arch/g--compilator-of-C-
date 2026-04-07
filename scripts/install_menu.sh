#!/usr/bin/env bash
set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PREFIX_DEFAULT="/data/data/com.termux/files/usr"
PREFIX="${PREFIX:-$PREFIX_DEFAULT}"
BINDIR="${BINDIR:-$PREFIX/bin}"
CFG_DIR="${GEE_CFG_DIR:-$PREFIX/etc/gee}"

choice="${1:-}"
if [[ -z "$choice" ]]; then
  cat <<MENU
Seleccione arquitectura de frontend para C!:
  1) x86
  2) x86-64
  3) ARM-v7
  4) ARM-64
MENU
  read -r -p "Opción [1-4]: " choice
fi

case "$choice" in
  1|x86) target="x86" ;;
  2|x86-64|x86_64) target="x86-64" ;;
  3|arm-v7|armv7) target="arm-v7" ;;
  4|arm-64|arm64|aarch64) target="arm-64" ;;
  *)
    echo "Opción inválida: $choice" >&2
    exit 2
    ;;
esac

mkdir -p "$BINDIR" "$CFG_DIR"

if [[ ! -x "$REPO_DIR/gee" ]]; then
  echo "Construyendo gee..."
  make -C "$REPO_DIR" stage0
fi

cp "$REPO_DIR/gee" "$BINDIR/gee-core"
chmod 755 "$BINDIR/gee-core"
cp "$REPO_DIR/scripts/gee-frontend.sh" "$BINDIR/gee"
cp "$REPO_DIR/scripts/gee-target.sh" "$BINDIR/gee-target"
cp "$REPO_DIR/scripts/gee-doctor.sh" "$BINDIR/gee-doctor"
chmod 755 "$BINDIR/gee"
chmod 755 "$BINDIR/gee-target" "$BINDIR/gee-doctor"

echo "$target" > "$CFG_DIR/target"

for t in x86 x86-64 arm-v7 arm-64; do
  cat > "$BINDIR/gee-$t" <<WRAP
#!/usr/bin/env bash
GEE_TARGET=$t exec "$BINDIR/gee" "\$@"
WRAP
  chmod 755 "$BINDIR/gee-$t"
done

echo "Instalación completada."
echo "Frontend activo: $target"
echo "Comando principal: $BINDIR/gee"
echo "Wrappers: gee-x86, gee-x86-64, gee-arm-v7, gee-arm-64"
echo "Utilidades: gee-target, gee-doctor"
