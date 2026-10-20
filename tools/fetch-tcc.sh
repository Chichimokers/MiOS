#!/usr/bin/env bash
# fetch-tcc.sh - Descarga las fuentes de tcc (Tiny C Compiler) en third_party/tcc.
#
# No hace falta 'make' ni binutils: el /bin/tcc de MiOS se compila con mios-gcc
# desde tools/build-tcc.sh (ver docs/PLAN-TOOLCHAIN.md, Fase 5).
#
# La version queda fijada (release_0_9_27) para que el build sea reproducible.
# third_party/ esta en .gitignore: las fuentes se bajan, no se versionan.
set -Eeuo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
DEST="$ROOT/third_party/tcc"
TAG="${TCC_TAG:-release_0_9_27}"

if [[ -f "$DEST/tcc.h" ]]; then
    echo "[tcc] las fuentes ya estan en $DEST"
    exit 0
fi

mkdir -p "$ROOT/third_party"
rm -rf "$DEST"

intento() {
    local url="$1" desc="$2"
    echo "[tcc] intento: $desc"
    if git clone --depth 1 --branch "$TAG" "$url" "$DEST" 2>/dev/null; then
        echo "[tcc] OK ($desc)"
        return 0
    fi
    rm -rf "$DEST"
    return 1
}

intento "https://github.com/TinyCC/tinycc.git" "GitHub (TinyCC/tinycc, $TAG)" && exit 0
intento "https://repo.or.cz/tinycc.git" "repo.or.cz ($TAG)" && exit 0

echo "[tcc] los clones han fallado; pruebo el tarball de savannah (necesita bzip2)"
TB="$ROOT/third_party/tcc-0.9.27.tar.bz2"
if curl -fsSL --max-time 120 -o "$TB" \
        "https://download.savannah.nongnu.org/releases/tinycc/tcc-0.9.27.tar.bz2"; then
    if ! command -v bzip2 >/dev/null 2>&1; then
        echo "[tcc] falta bzip2: instala con 'sudo apt-get install -y bzip2' y repite"
        exit 1
    fi
    tar -xjf "$TB" -C "$ROOT/third_party"
    mv "$ROOT/third_party/tcc-0.9.27" "$DEST"
    rm -f "$TB"
    echo "[tcc] OK (tarball de savannah)"
    exit 0
fi

echo "[tcc] no he podido descargar tcc" >&2
exit 1
