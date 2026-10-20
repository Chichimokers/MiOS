#!/usr/bin/env bash
#
# test-fase2.sh - compila y ejecuta examples/fase2.c en el VPS (chroot del
# sysroot, sin tocar el sistema anfitrion; el programa no monta nada).
#
#   uso:  bash test-fase2.sh
#
set -uo pipefail

ROOT="/home/chichimokers/OS Nuevo test"
cd "$ROOT" || exit 1

mkdir -p sysroot/tmp
chmod +x mios-gcc mk-sysroot.sh 2>/dev/null || true

echo "--- compilando examples/fase2.c ---"
./mios-gcc examples/fase2.c -o build/fase2 || exit 1

echo "--- relocalizaciones (debe haber R_X86_64_COPY para stdout/environ) ---"
readelf -r build/fase2 | grep -E 'COPY|JUMP_SLOT' | head -20

cp -f build/fase2 sysroot/fase2

echo
echo "--- ejecutando dentro del chroot del sysroot ---"
echo 123 | sudo -S chroot sysroot /fase2
echo "codigo de salida = $?"
