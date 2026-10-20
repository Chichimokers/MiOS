#!/usr/bin/env bash
# fetch-gcc-libs.sh - Descarga GMP, MPFR y MPC en third_party/ (Fase 8).
#
# Son las tres bibliotecas aritmeticas que necesita GCC.  En vez de dejar que
# GCC se las construya dentro de su propio arbol (que es lo que hace
# 'contrib/download_prerequisites'), las construimos APARTE como bibliotecas
# compartidas de MiOS: asi 'gcc' y 'cc1' pueden cargarlas desde /lib dentro de
# MiOS y podemos probar cada una por su cuenta antes de pelear con GCC.
#
# Las versiones son EXACTAMENTE las que GCC 13.3 espera (las de su
# contrib/download_prerequisites), para no mezclar combinaciones no probadas:
#
#   gmp-6.2.1   mpfr-4.1.0   mpc-1.2.1
#
# third_party/ esta en .gitignore: las fuentes se bajan, no se versionan.
set -Eeuo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
DEST="$ROOT/third_party"

GMP_VER="${GMP_VERSION:-6.2.1}"
MPFR_VER="${MPFR_VERSION:-4.1.0}"
MPC_VER="${MPC_VERSION:-1.2.1}"

mkdir -p "$DEST"

# descarga <url-base> <url-base-alt> <fichero> <directorio-destino> <subdir-clave>
descarga() {
    local urls=("$1" "$2") file="$3" dir="$4" clave="$5"
    if [[ -f "$dir/$clave" ]]; then
        echo "[gcc-libs] $dir ya esta"
        return 0
    fi
    rm -rf "$dir"
    local tar="$DEST/$file"
    local ok=0 u
    for u in "${urls[@]}"; do
        echo "[gcc-libs] descargando $file de $u ..."
        if curl -fsSL --max-time 600 -o "$tar" "$u"; then ok=1; break; fi
    done
    [[ "$ok" == "1" ]] || { echo "[gcc-libs] no he podido descargar $file" >&2; exit 1; }

    local tmp="$DEST/.tmp-$$"
    rm -rf "$tmp"; mkdir -p "$tmp"
    case "$file" in
    *.tar.xz) tar -xJf "$tar" -C "$tmp" ;;
    *.tar.bz2) tar -xjf "$tar" -C "$tmp" ;;
    *.tar.gz) tar -xzf "$tar" -C "$tmp" ;;
    esac
    local extraido
    extraido="$(find "$tmp" -maxdepth 1 -mindepth 1 -type d | head -n 1)"
    [[ -n "$extraido" ]] || { echo "[gcc-libs] $file no contiene un directorio" >&2; exit 1; }
    mv "$extraido" "$dir"
    rm -rf "$tmp"
    echo "[gcc-libs] listo: $dir"
}

descarga \
    "https://ftp.gnu.org/gnu/gmp/gmp-$GMP_VER.tar.xz" \
    "https://mirrors.kernel.org/gnu/gmp/gmp-$GMP_VER.tar.xz" \
    "gmp-$GMP_VER.tar.xz" "$DEST/gmp" "gmp-h.in"

descarga \
    "https://ftp.gnu.org/gnu/mpfr/mpfr-$MPFR_VER.tar.xz" \
    "https://mirrors.kernel.org/gnu/mpfr/mpfr-$MPFR_VER.tar.xz" \
    "mpfr-$MPFR_VER.tar.xz" "$DEST/mpfr" "mpfr.h.in"

descarga \
    "https://ftp.gnu.org/gnu/mpc/mpc-$MPC_VER.tar.gz" \
    "https://mirrors.kernel.org/gnu/mpc/mpc-$MPC_VER.tar.gz" \
    "mpc-$MPC_VER.tar.gz" "$DEST/mpc" "mpc.h.in"

echo
echo "[gcc-libs] versiones instaladas en third_party/:"
for p in gmp mpfr mpc; do
    if [[ -f "$DEST/$p/configure" ]]; then
        echo "  $p: $(grep -m1 -E "^ *PACKAGE_VERSION=|^ *VERSION=" "$DEST/$p/configure" 2>/dev/null | head -n 1)"
    fi
done
