#!/usr/bin/env bash
#
# test-fase3.sh - compila y ejecuta examples/fase3.c en el VPS (chroot del
# sysroot, sin tocar el sistema anfitrion; el programa no monta nada).
#
# Enlaza con -lm a proposito: demuestra que el sysroot tiene libm.so apuntando
# a libmios.so.6.
#
#   uso:  bash test-fase3.sh
#
set -uo pipefail

ROOT="/home/chichimokers/OS Nuevo test"
cd "$ROOT" || exit 1

mkdir -p sysroot/tmp
chmod +x mios-gcc mk-sysroot.sh 2>/dev/null || true

echo "--- compilando examples/fase3.c (con -lm) ---"
./mios-gcc examples/fase3.c -o build/fase3 -lm || exit 1

echo "--- dependencias del ejecutable ---"
readelf -d build/fase3 | grep -E 'NEEDED' || true

cp -f build/fase3 sysroot/fase3

echo
echo "--- ejecutando dentro del chroot del sysroot ---"
echo 123 | sudo -S chroot sysroot /fase3
echo "codigo de salida = $?"
