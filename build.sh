#!/usr/bin/env bash
set -Eeuo pipefail

# Directorio donde se encuentra este script
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

echo "=================================="
echo "          Construyendo MiOS"
echo "=================================="
echo "Proyecto: $ROOT"

# Comprobar herramientas
for cmd in gcc cpio gzip grub-mkrescue; do
    if ! command -v "$cmd" >/dev/null 2>&1; then
        echo "ERROR: falta '$cmd'."
        echo "Instala las dependencias necesarias."
        exit 1
    fi
done

# Comprobar archivos requeridos
for file in \
    src/core/main.c \
    src/ld/ldmios.c \
    src/crt/crt1.c \
    initramfs/dev/console \
    iso/boot/vmlinuz \
    iso/boot/grub/grub.cfg
do
    if [[ ! -e "$ROOT/$file" ]]; then
        echo "ERROR: no existe $file"
        if [[ "$file" == "initramfs/dev/console" ]]; then
            echo "Crea el nodo de consola con:  sudo mknod initramfs/dev/console c 5 1"
        fi
        exit 1
    fi
done

BUILD="$ROOT/build"
rm -rf "$BUILD"
mkdir -p "$BUILD/lib" "$BUILD/exe" "$BUILD/ld" "$BUILD/bin"

# Flags comunes a todo el codigo de MiOS (sin libc, freestanding).
COMMON=(
    -ffreestanding -fno-builtin -fno-stack-protector -fcf-protection=none
    -O2 -Wall -Wextra -I"$ROOT/src"
)

# Nombres de objeto sin colisiones (ruta -> nombre_plano).
objname() { local p="$1"; p="${p#"$ROOT"/}"; echo "${p//\//_}"; }

# ------------------------------------------------------------------
# 1. libmios.so.6 : la libc propia (src/lib + la capa de syscalls)
# ------------------------------------------------------------------
echo
echo "[1/11] Compilando libmios.so.6 (libc propia, PIC)..."

mapfile -t LIB_SOURCES < <(
    { find "$ROOT/src/lib" -type f -name '*.c'
      find "$ROOT/src/kernel" -type f -name '*.c'; } | sort
)

for src in "${LIB_SOURCES[@]}"; do
    gcc "${COMMON[@]}" -fPIC -c "$src" -o "$BUILD/lib/$(objname "$src").o"
done

gcc -shared -nostdlib -Wl,-soname,libmios.so.6 -Wl,--hash-style=sysv \
    -Wl,-z,noexecstack \
    -o "$BUILD/lib/libmios.so.6" "$BUILD"/lib/*.o

ln -sf libmios.so.6 "$BUILD/lib/libmios.so"

# ------------------------------------------------------------------
# 2. ld-mios.so.1 : el cargador dinamico propio (estatico, sin deps)
# ------------------------------------------------------------------
echo
echo "[2/11] Compilando ld-mios.so.1 (cargador propio)..."

# El cargador es ET_EXEC y se coloca lejos del ejecutable (0x400000) para no
# solaparse con el. Se enlaza estatico: no depende de libmios.
gcc "${COMMON[@]}" -nostdlib -static -no-pie -fno-pie \
    -Wl,-e,_start -Wl,-z,noexecstack -Wl,-Ttext-segment=0x40000000 \
    "$ROOT/src/ld/ldmios.c" \
    -o "$BUILD/ld/ld-mios.so.1" \
    -lgcc

# ------------------------------------------------------------------
# 3. init : el ejecutable (core + fs + net) enlazado contra libmios
# ------------------------------------------------------------------
echo
echo "[3/11] Compilando y enlazando init (dinamico)..."

mapfile -t EXE_SOURCES < <(
    find "$ROOT/src/core" "$ROOT/src/fs" "$ROOT/src/net" \
        -type f -name '*.c' | sort
)

for src in "${EXE_SOURCES[@]}"; do
    gcc "${COMMON[@]}" -fno-pie -c "$src" -o "$BUILD/exe/$(objname "$src").o"
done

gcc -nostdlib -no-pie -fno-pie \
    -Wl,-e,_start \
    -Wl,--dynamic-linker=/lib/ld-mios.so.1 \
    -Wl,-z,noexecstack \
    -L"$BUILD/lib" \
    "$BUILD"/exe/*.o \
    -lmios -lgcc \
    -o "$ROOT/initramfs/init"

chmod +x "$ROOT/initramfs/init"

# ------------------------------------------------------------------
# 4. Sysroot + mios-gcc : hace falta antes de compilar los programas
# ------------------------------------------------------------------
echo
echo "[4/11] Montando el sysroot y probando mios-gcc..."

bash "$ROOT/mk-sysroot.sh"
chmod +x "$ROOT/mios-gcc" "$ROOT/mk-sysroot.sh"

# ------------------------------------------------------------------
# 5. Programas de usuario (/bin) que el shell puede lanzar (Fase 4)
# ------------------------------------------------------------------
echo
echo "[5/11] Compilando los programas de /bin (userland/)..."

mkdir -p "$ROOT/initramfs/bin" "$ROOT/initramfs/tmp"

for src in "$ROOT"/userland/*.c; do
    [[ -e "$src" ]] || continue
    name="$(basename "$src" .c)"
    "$ROOT/mios-gcc" "$src" -o "$ROOT/initramfs/bin/$name"
done

ls -l "$ROOT/initramfs/bin"

# ------------------------------------------------------------------
# 6. tcc : el compilador de C que corre DENTRO de MiOS (Fases 5 y 6)
# ------------------------------------------------------------------
echo
echo "[6/11] Compilando tcc (compilador de C dentro de MiOS)..."

if [[ -f "$ROOT/third_party/tcc/tcc.h" ]]; then
    bash "$ROOT/tools/build-tcc.sh"
else
    echo "  (sin fuentes de tcc: ejecuta 'bash tools/fetch-tcc.sh' para incluirlo)"
    echo "  (se sigue sin tcc; el resto de MiOS no depende de el)"
fi

# ------------------------------------------------------------------
# 7. binutils : el as y el ld de MiOS (Fase 7), para GCC mas adelante
# ------------------------------------------------------------------
echo
echo "[7/11] Compilando binutils (as y ld para MiOS)..."

if [[ -f "$ROOT/third_party/binutils/gas/as.c" ]]; then
    if ! bash "$ROOT/tools/build-binutils.sh"; then
        echo "  (binutils no se ha podido construir; se sigue sin as/ld)"
        echo "  (revisa tools/build-binutils.sh: hace falta 'make' y"
        echo "   'sudo bash tools/mios-host-run.sh' en el VPS)"
    fi
else
    echo "  (sin fuentes de binutils: 'bash tools/fetch-binutils.sh' para incluirlo)"
    echo "  (se sigue sin as/ld; MiOS arranca igual)"
fi

# ------------------------------------------------------------------
# 8. GMP + MPFR + MPC : las bibliotecas aritmeticas de GCC (Fase 8)
# ------------------------------------------------------------------
echo
echo "[8/11] Compilando GMP, MPFR y MPC (bibliotecas de GCC)..."

if [[ -f "$ROOT/third_party/gmp/configure" \
   && -f "$ROOT/third_party/mpfr/configure" \
   && -f "$ROOT/third_party/mpc/configure" ]]; then
    if ! bash "$ROOT/tools/build-gcc-libs.sh"; then
        echo "  (GMP/MPFR/MPC no se han podido construir; se sigue sin ellas)"
        echo "  (revisa tools/build-gcc-libs.sh: hacen falta 'make', 'm4' y"
        echo "   'sudo bash tools/mios-host-run.sh' en el VPS)"
    fi
else
    echo "  (sin fuentes: 'bash tools/fetch-gcc-libs.sh' para bajarlas)"
    echo "  (se sigue sin libgmp/libmpfr/libmpc; MiOS arranca igual)"
fi

# ------------------------------------------------------------------
# 9. Instalar la libc y el cargador en el initramfs
# ------------------------------------------------------------------
echo
echo "[9/11] Instalando libmios.so.6 y ld-mios.so.1 en initramfs/lib/..."

mkdir -p "$ROOT/initramfs/lib"
cp -f "$BUILD/lib/libmios.so.6" "$ROOT/initramfs/lib/libmios.so.6"
cp -f "$BUILD/ld/ld-mios.so.1"  "$ROOT/initramfs/lib/ld-mios.so.1"
# libmios.so (sin version) para que 'ld -lmios' la encuentre (Fase 7).
ln -sf libmios.so.6 "$ROOT/initramfs/lib/libmios.so"

# ------------------------------------------------------------------
# 10. Empaquetar initramfs y generar la ISO
# ------------------------------------------------------------------
echo
echo "[10/11] Empaquetando initramfs y generando MiOS.iso..."

# Saneamiento de permisos: 'cp -f' conserva el modo del fichero destino, y el
# tar que viene de Windows no trae el bit +x.  Si 'lib/ld-mios.so.1' queda en
# 0664 el kernel no puede ejecutar el interprete de /init y el arranque muere
# con "Failed to execute /init (error -13)" + Kernel panic.  Nos aseguramos de
# que lo que se empaqueta es ejecutable.
chmod 755 "$ROOT/initramfs/init"
if [ -f "$ROOT/initramfs/lib/ld-mios.so.1" ]; then
    chmod 755 "$ROOT/initramfs/lib/ld-mios.so.1"
fi
if [ -d "$ROOT/initramfs/bin" ]; then
    find "$ROOT/initramfs/bin" -type f -exec chmod 755 {} +
fi

TMP_ARCHIVE="$(mktemp "$ROOT/iso/boot/.initramfs.XXXXXX")"
trap 'rm -f "${TMP_ARCHIVE:-}"' EXIT

(
    cd "$ROOT/initramfs"
    find . -print0 |
        cpio --null -o --format=newc --quiet |
        gzip -9
) > "$TMP_ARCHIVE"

gzip -t "$TMP_ARCHIVE"
mv -f "$TMP_ARCHIVE" "$ROOT/iso/boot/initramfs.cpio.gz"
TMP_ARCHIVE=""

file "$ROOT/initramfs/init"
file "$ROOT/initramfs/lib/libmios.so.6"
ls -lh "$ROOT/iso/boot/vmlinuz"
ls -lh "$ROOT/iso/boot/initramfs.cpio.gz"

grub-mkrescue -o "$ROOT/MiOS.iso" "$ROOT/iso"

# ------------------------------------------------------------------
# 11. Comprobacion final: un programa se compila contra el sysroot
# ------------------------------------------------------------------
echo
echo "[11/11] Comprobando mios-gcc con examples/hola.c..."

"$ROOT/mios-gcc" "$ROOT/examples/hola.c" -o "$BUILD/hola"

echo
echo "  Programa de prueba (ejecutable dinamico contra libmios):"
file "$BUILD/hola"
readelf -d "$BUILD/hola" | grep -E 'NEEDED' || true

echo
echo "=================================="
echo "BUILD COMPLETADO"
echo "ISO: $ROOT/MiOS.iso"
ls -lh "$ROOT/MiOS.iso"
echo
echo "Para compilar tus programas:  ./mios-gcc ejemplo.c -o ejemplo"
echo "=================================="
