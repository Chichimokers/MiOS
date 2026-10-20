#!/usr/bin/env bash
# fetch-gcc.sh - Descarga las fuentes de GCC 13.3 en third_party/gcc (Fase 9).
#
# La version queda fijada a la 13.3 (la misma que trae Ubuntu 24.04 como gcc
# del sistema), para que las herramientas que construimos a mano (binutils 2.42,
# GMP 6.2.1, MPFR 4.1.0, MPC 1.2.1) sean exactamente las que ese GCC espera.
#
# third_party/ esta en .gitignore: las fuentes se bajan, no se versionan.
set -Eeuo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
VER="${GCC_VERSION:-13.3.0}"
DEST="$ROOT/third_party/gcc"
TAR="$ROOT/third_party/gcc-$VER.tar.xz"

if [[ -f "$DEST/gcc/cc1.c" || -f "$DEST/gcc/cc1.cc" || -f "$DEST/configure" ]]; then
    echo "[gcc] las fuentes ya estan en $DEST"
    exit 0
fi

mkdir -p "$ROOT/third_party"
rm -rf "$DEST"

# Ojo con el espejo: ftp.gnu.org iba a 11 KB/s desde el VPS (2 h de descarga) y
# mirrors.kernel.org va a velocidad normal; se prueban en ese orden.  Una
# descarga a medias se sobrescribe (no se reanuda), asi que no hay sorpresas.
echo "[gcc] descargando gcc-$VER (unos 84 MB, tarda un rato)..."
urls=(
    "https://mirrors.kernel.org/gnu/gcc/gcc-$VER/gcc-$VER.tar.xz"
    "https://ftp.gnu.org/gnu/gcc/gcc-$VER/gcc-$VER.tar.xz"
)
ok=0
for u in "${urls[@]}"; do
    echo "[gcc] probando $u"
    if curl -fSL --retry 3 --retry-delay 5 --max-time 3600 -o "$TAR" "$u"; then ok=1; break; fi
done
[[ "$ok" == "1" ]] || { echo "[gcc] no he podido descargar gcc-$VER" >&2; exit 1; }

echo "[gcc] extrayendo..."
tmp="$(mktemp -d "$ROOT/third_party/.gcc.XXXXXX")"
tar -xJf "$TAR" -C "$tmp"
mv "$tmp/gcc-$VER" "$DEST"
rm -rf "$tmp"

echo "[gcc] listo: $DEST ($(du -sh "$DEST" | cut -f1))"
