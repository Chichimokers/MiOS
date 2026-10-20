#!/usr/bin/env bash
#
# test-fase4.sh - compila y ejecuta examples/fase4.c en el VPS (chroot del
# sysroot).  Antes copia los programas de userland/ (hola, args) al chroot,
# que es lo que el shell de MiOS lanzaria desde /bin.
#
# Los programas no montan nada, asi que el chroot es seguro.
#
#   uso:  bash test-fase4.sh
#
set -uo pipefail

ROOT="/home/chichimokers/OS Nuevo test"
cd "$ROOT" || exit 1

mkdir -p sysroot/tmp sysroot/bin
chmod +x mios-gcc mk-sysroot.sh 2>/dev/null || true

echo "--- compilando userland/ y examples/fase4.c ---"
./mios-gcc userland/hola.c -o build/user-hola || exit 1
./mios-gcc userland/args.c -o build/user-args || exit 1
./mios-gcc examples/fase4.c -o build/fase4 || exit 1

# /hola y /args (ruta absoluta) y tambien en /bin (para PATH).
cp -f build/user-hola sysroot/hola
cp -f build/user-hola sysroot/bin/hola
cp -f build/user-args sysroot/args
cp -f build/user-args sysroot/bin/args
cp -f build/fase4 sysroot/fase4

echo
echo "--- ejecutando dentro del chroot del sysroot ---"
echo 123 | sudo -S chroot sysroot /fase4
echo "codigo de salida = $?"
