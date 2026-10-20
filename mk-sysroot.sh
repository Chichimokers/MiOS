#!/usr/bin/env bash
#
# mk-sysroot.sh - monta el sysroot de MiOS (Fase 1).
#
# Deja en sysroot/ todo lo necesario para compilar y ejecutar programas:
#
#   sysroot/usr/include/  cabeceras publicas (las de include/*.h)
#   sysroot/usr/lib/      crt1.o, libmios.so.6 (+ enlace .so) y ld-mios.so.1
#   sysroot/lib/          lo mismo (el cargador busca /lib/... en ejecucion)
#
# El sysroot sirve ademas como raiz ejecutable, asi que se puede probar un
# binario sin tocar el sistema del VPS:
#
#   cp hola sysroot/ && sudo chroot sysroot /hola
#
# Requiere haber ejecutado antes ./build.sh (usa los artefactos de build/).

set -Eeuo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
SYSROOT="${MIOS_SYSROOT:-$ROOT/sysroot}"
BUILD="$ROOT/build"

CC="${MIOS_CC:-gcc}"

# Flags con los que se compila el codigo de MiOS (sin libc del sistema).
CRT_FLAGS=(
    -ffreestanding -fno-builtin -fno-stack-protector -fcf-protection=none
    -O2 -Wall -Wextra -fno-pie
)

echo "[sysroot] Preparando $SYSROOT ..."

if [[ ! -f "$BUILD/lib/libmios.so.6" || ! -f "$BUILD/ld/ld-mios.so.1" ]]; then
    echo "ERROR: faltan artefactos en build/." >&2
    echo "       Ejecuta ./build.sh primero." >&2
    exit 1
fi

rm -rf "$SYSROOT"
mkdir -p "$SYSROOT/usr/include" "$SYSROOT/usr/lib" "$SYSROOT/lib"

# 1. Cabeceras publicas (incluye subdirectorios como sys/).
cp -a "$ROOT"/include/. "$SYSROOT/usr/include/"

# 2. crt1.o: el _start de los programas normales (argc/argv -> main -> exit).
"$CC" "${CRT_FLAGS[@]}" -c "$ROOT/src/crt/crt1.c" -o "$SYSROOT/usr/lib/crt1.o"

# 3. La libc propia y el cargador.
cp -f "$BUILD/lib/libmios.so.6" "$SYSROOT/usr/lib/libmios.so.6"
cp -f "$BUILD/ld/ld-mios.so.1"  "$SYSROOT/usr/lib/ld-mios.so.1"
ln -sf libmios.so.6 "$SYSROOT/usr/lib/libmios.so"

# 3b. libm y libc apuntan a la misma biblioteca: -lm y -lc tienen que encontrar
# algo en el sysroot (el cargador registra el SONAME libmios.so.6 de todas formas).
ln -sf libmios.so.6 "$SYSROOT/usr/lib/libm.so"
ln -sf libmios.so.6 "$SYSROOT/usr/lib/libc.so"

# 4. Espejo en /lib: son las rutas que busca el cargador en tiempo de ejecucion.
cp -f "$SYSROOT/usr/lib/libmios.so.6" "$SYSROOT/lib/libmios.so.6"
cp -f "$SYSROOT/usr/lib/ld-mios.so.1"  "$SYSROOT/lib/ld-mios.so.1"
ln -sf libmios.so.6 "$SYSROOT/lib/libmios.so"

echo "[sysroot] Cabeceras: $(ls "$SYSROOT/usr/include" | tr '\n' ' ')"
echo "[sysroot] Listo para usar con ./mios-gcc"
