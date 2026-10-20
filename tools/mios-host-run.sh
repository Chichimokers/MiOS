#!/usr/bin/env bash
# mios-host-run.sh - Prepara el VPS para poder EJECUTAR binarios de MiOS.
#
# Por que hace falta: el ABI de syscalls de MiOS es exactamente el de Linux
# x86_64, asi que un binario de MiOS corre igual en el VPS.  Solo le faltan dos
# ficheros en las rutas que pide su cargador:
#
#   /lib/ld-mios.so.1    -> $ROOT/sysroot/lib/ld-mios.so.1
#   /lib/libmios.so.6    -> $ROOT/sysroot/lib/libmios.so.6
#
# Con esos dos enlaces, las pruebas de 'configure' y 'make' de binutils (y mas
# adelante las de gcc) se ejecutan de verdad, sin chroot y sin config.cache.
# Son enlaces a rutas del repositorio, asi que no se quedan obsoletos: cada
# build.sh los refresca en el sysroot.
#
# Uso:  sudo bash tools/mios-host-run.sh --list
#       sudo bash tools/mios-host-run.sh            (crea/actualiza los enlaces)
#       sudo bash tools/mios-host-run.sh --remove   (los quita)
#
# Nota: solo afecta a binarios que pidan 'ld-mios.so.1' (los nuestros). Ningun
# programa del VPS usa esos nombres, asi que no interfiere con el sistema.
set -Eeuo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
SYSROOT="$ROOT/sysroot"

# /lib suele ser un enlace a /usr/lib (usrmerge); se escribe siempre en /usr/lib
# para tener una unica ruta real.
DIR="/usr/lib"
LINKS=(
    "ld-mios.so.1:sysroot/lib/ld-mios.so.1"
    "libmios.so.6:sysroot/lib/libmios.so.6"
)

accion="${1:---create}"

case "$accion" in
--list)
    for l in "${LINKS[@]}"; do
        n="${l%%:*}"
        p="${l#*:}"
        if [[ -L "$DIR/$n" ]]; then
            echo "$DIR/$n -> $(readlink "$DIR/$n")   [$( [[ -e "$DIR/$n" ]] && echo ok || echo ROTO )]"
        elif [[ -e "$DIR/$n" ]]; then
            echo "$DIR/$n (fichero normal, NO es nuestro)"
        else
            echo "$DIR/$n (no existe)"
        fi
    done
    exit 0
    ;;
--remove)
    for l in "${LINKS[@]}"; do
        n="${l%%:*}"
        if [[ -L "$DIR/$n" ]]; then
            rm -f "$DIR/$n"
            echo "quitado: $DIR/$n"
        fi
    done
    echo "[host-run] el VPS ya no ejecuta binarios de MiOS."
    exit 0
    ;;
--create) ;;
*)
    echo "uso: sudo bash tools/mios-host-run.sh [--create|--list|--remove]" >&2
    exit 2
    ;;
esac

if [[ ! -f "$SYSROOT/lib/ld-mios.so.1" || ! -f "$SYSROOT/lib/libmios.so.6" ]]; then
    echo "[host-run] falta el sysroot ($SYSROOT/lib).  Ejecuta ./build.sh primero." >&2
    exit 1
fi

for l in "${LINKS[@]}"; do
    n="${l%%:*}"
    p="${l#*:}"
    dest="$ROOT/$p"
    if [[ -e "$DIR/$n" && ! -L "$DIR/$n" ]]; then
        echo "[host-run] $DIR/$n existe y NO es un enlace nuestro: no lo toco." >&2
        continue
    fi
    ln -sfn "$dest" "$DIR/$n"
    echo "[host-run] $DIR/$n -> $dest"
done

echo
echo "[host-run] prueba:"
cat > /tmp/mios-host-run.c <<'EOF'
#include <stdio.h>
int main(void) { printf("miOS corriendo en el VPS\n"); return 0; }
EOF
if [[ -x "$ROOT/mios-gcc" || -f "$ROOT/mios-gcc" ]]; then
    if bash "$ROOT/mios-gcc" /tmp/mios-host-run.c -o /tmp/mios-host-run 2>/dev/null; then
        /tmp/mios-host-run && echo "[host-run] OK" || echo "[host-run] el binario NO arranca"
    fi
fi
