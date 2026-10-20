#!/usr/bin/env bash
# fetch-binutils.sh - Descarga las fuentes de binutils en third_party/binutils.
#
# De aqui sale el 'as' y el 'ld' de MiOS (Fase 7).  A diferencia de tcc, binutils
# no se compila a mano: tools/build-binutils.sh usa su propio 'configure' y
# 'make', asi que hacen falta las fuentes completas y la herramienta make en el
# VPS (sudo apt install make).
#
# La version queda fijada (2.42, la misma que trae Ubuntu 24.04) para que el
# build sea reproducible y para que combine con el gcc de la Fase 9.
# third_party/ esta en .gitignore: las fuentes se bajan, no se versionan.
set -Eeuo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
VER="${BINUTILS_VERSION:-2.42}"
DEST="$ROOT/third_party/binutils"
TAR="$ROOT/third_party/binutils-$VER.tar.xz"

if [[ -f "$DEST/gas/as.c" ]]; then
    echo "[binutils] las fuentes ya estan en $DEST"
    exit 0
fi

mkdir -p "$ROOT/third_party"
rm -rf "$DEST"

echo "[binutils] descargando binutils-$VER..."
urls=(
    "https://ftp.gnu.org/gnu/binutils/binutils-$VER.tar.xz"
    "https://mirrors.kernel.org/gnu/binutils/binutils-$VER.tar.xz"
    "https://ftp.fu-berlin.de/gnu/binutils/binutils-$VER.tar.xz"
)
ok=0
for u in "${urls[@]}"; do
    echo "[binutils] intento: $u"
    if curl -fsSL --max-time 600 -o "$TAR" "$u"; then
        ok=1
        break
    fi
done
if [[ "$ok" != "1" ]]; then
    echo "[binutils] no he podido descargar binutils-$VER" >&2
    exit 1
fi

echo "[binutils] extrayendo (tarda un poco)..."
tar -xJf "$TAR" -C "$ROOT/third_party"
mv "$ROOT/third_party/binutils-$VER" "$DEST"
rm -f "$TAR"

if [[ ! -f "$DEST/gas/as.c" ]]; then
    echo "[binutils] el arbol extraido no parece binutils" >&2
    exit 1
fi

echo "[binutils] OK: $DEST"
echo "[binutils] generadores incluidos en el tarball:"
for f in ld/ldgram.c ld/ldlex.c ld/ldgram.h opcodes/i386-dis.c; do
    if [[ -f "$DEST/$f" ]]; then
        echo "  si  $f"
    else
        echo "  NO  $f   (haria falta bison/flex)"
    fi
done
