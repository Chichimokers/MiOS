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
    initramfs/dev/console \
    iso/boot/vmlinuz \
    iso/boot/grub/grub.cfg
do
    if [[ ! -e "$ROOT/$file" ]]; then
        echo "ERROR: no existe $file"
        exit 1
    fi
done

# 1. Compilar MiOS (sin libc, solo kernel)
echo
echo "[1/4] Compilando MiOS (C sobre el kernel)..."

mapfile -t SOURCES < <(find "$ROOT/src" -type f \( -name '*.c' -o -name '*.S' \) | sort)

if [[ ${#SOURCES[@]} -eq 0 ]]; then
    echo "ERROR: no hay fuentes en src/"
    exit 1
fi

gcc -static -nostdlib -no-pie -fno-pie -ffreestanding \
    -fno-builtin -fno-stack-protector -fcf-protection=none \
    -O2 -Wall -Wextra \
    -I"$ROOT/src" \
    "${SOURCES[@]}" \
    -o "$ROOT/initramfs/init" \
    -Wl,-e,_start -Wl,-z,noexecstack \
    -lgcc

chmod +x "$ROOT/initramfs/init"

# 2. Empaquetar initramfs
echo
echo "[2/4] Generando initramfs.cpio.gz..."

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

# 3. Verificar ejecutable y archivos
echo
echo "[3/4] Verificando..."

file "$ROOT/initramfs/init"
ls -lh "$ROOT/iso/boot/vmlinuz"
ls -lh "$ROOT/iso/boot/initramfs.cpio.gz"

# 4. Generar ISO
echo
echo "[4/4] Generando MiOS.iso..."

grub-mkrescue -o "$ROOT/MiOS.iso" "$ROOT/iso"

echo
echo "=================================="
echo "BUILD COMPLETADO"
echo "ISO: $ROOT/MiOS.iso"
ls -lh "$ROOT/MiOS.iso"
echo "=================================="