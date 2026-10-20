#!/usr/bin/env bash
#
# sync-vps.sh — Sincroniza este repositorio con el VPS Linux de build,
# para que Windows y el VPS queden iguales.
#
#   Origen (esta máquina)  : carpeta del repo
#   Destino                : chichimokers@192.168.1.87:"/home/chichimokers/OS Nuevo test"
#
# Uso:  ./sync-vps.sh
#
# Notas:
#   - Se ejecuta desde Git Bash (Windows) o desde Linux.
#   - Si tienes 'sshpass' instalado, no pedirá la contraseña; si no, la pedirá (123).
#   - Para no teclear la clave nunca, configura una clave SSH una sola vez:
#       ssh-keygen -t ed25519
#       ssh-copy-id chichimokers@192.168.1.87
#
set -Eeuo pipefail

VPS_HOST="192.168.1.87"
VPS_USER="chichimokers"
VPS_PASS="123"
# Carpeta espejo en el VPS (tal cual esta): /home/chichimokers/OS Nuevo test
VPS_DIR="/home/chichimokers/OS Nuevo test"

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

TARGET="$VPS_USER@$VPS_HOST"
SSH_OPTS=(-o StrictHostKeyChecking=accept-new -o LogLevel=ERROR)
# Lo que NO se sube (lo genera el VPS; el pull lo trae aqui como espejo).
# Motivo de los artefactos de initramfs: subirlos de vuelta sustituiria los
# enlaces (libgmp.so -> libgmp.so.10.4.1, libc.so -> libmios.so.6) por copias
# del fichero entero, y el initramfs creceria sin necesidad.
EXCLUDES=(--exclude './MiOS.iso' --exclude './*.cpio.gz' --exclude './initramfs/init'
          --exclude './sysroot' --exclude './build' --exclude './mios-bin'
          --exclude './initramfs/bin' --exclude './initramfs/usr' --exclude './initramfs/lib/tcc'
          --exclude './initramfs/lib/libmios.so*' --exclude './initramfs/lib/libm.so*'
          --exclude './initramfs/lib/libc.so*' --exclude './initramfs/lib/ld-mios.so.1'
          --exclude './initramfs/lib/libgmp.so*' --exclude './initramfs/lib/libmpfr.so*'
          --exclude './initramfs/lib/libmpc.so*')

# Prefijo con sshpass si está disponible (evita el prompt de contraseña).
if command -v sshpass >/dev/null 2>&1; then
    SSH_PREFIX=(sshpass -p "$VPS_PASS")
else
    SSH_PREFIX=()
    echo "[aviso] 'sshpass' no está instalado: se pedirá la contraseña ($VPS_PASS) una o más veces."
fi

echo "[sync] destino: $TARGET:$VPS_DIR"

# Asegura que existe el directorio destino.
"${SSH_PREFIX[@]}" ssh "${SSH_OPTS[@]}" "$TARGET" "mkdir -p '$VPS_DIR'"

if command -v rsync >/dev/null 2>&1; then
    echo "[sync] usando rsync..."
    RSYNC_SSH="ssh ${SSH_OPTS[*]}"
    if [[ ${#SSH_PREFIX[@]} -gt 0 ]]; then
        RSYNC_SSH="sshpass -p $VPS_PASS ssh ${SSH_OPTS[*]}"
    fi
    # -s (protect args): imprescindible porque la ruta remota tiene espacios.
    rsync -az --delete -s -e "$RSYNC_SSH" \
        --exclude 'MiOS.iso' --exclude '*.cpio.gz' --exclude 'initramfs/init' \
        --exclude 'sysroot' --exclude 'build' --exclude 'mios-bin' \
        --exclude 'initramfs/bin' --exclude 'initramfs/usr' --exclude 'initramfs/lib/tcc' \
        --exclude 'initramfs/lib/libmios.so*' --exclude 'initramfs/lib/libm.so*' \
        --exclude 'initramfs/lib/libc.so*' --exclude 'initramfs/lib/ld-mios.so.1' \
        --exclude 'initramfs/lib/libgmp.so*' --exclude 'initramfs/lib/libmpfr.so*' \
        --exclude 'initramfs/lib/libmpc.so*' \
        "$ROOT/" "$TARGET:$VPS_DIR/"
else
    echo "[sync] 'rsync' no está disponible: usando tar sobre ssh..."
    tar -czf - "${EXCLUDES[@]}" . \
        | "${SSH_PREFIX[@]}" ssh "${SSH_OPTS[@]}" "$TARGET" \
            "mkdir -p '$VPS_DIR' && tar -xzf - -C '$VPS_DIR'"
fi

# Windows no conserva el bit +x al empaquetar (y Git Bash tampoco con rsync):
# se lo devolvemos a los scripts en el VPS.  Si no, 'build.sh' y 'mios-gcc'
# fallan con "Permission denied" tras cada sincronizacion.
"${SSH_PREFIX[@]}" ssh "${SSH_OPTS[@]}" "$TARGET" \
    "cd '$VPS_DIR' && chmod +x *.sh mios-gcc tools/*.sh 2>/dev/null || true"

echo "[ok] Sincronizado con $TARGET:$VPS_DIR"

# --- Y de vuelta: traer del VPS lo que haya cambiado alli (artefactos del build:
# initramfs/init, bin/, lib/, usr/, iso/...).  Se hace DESPUES de subir, para no
# pisar ningun cambio local sin subir.  Sin --delete: no borra nada de aqui.
if command -v rsync >/dev/null 2>&1; then
    echo "[sync] de vuelta (rsync): trayendo del VPS..."
    RSYNC_SSH="ssh ${SSH_OPTS[*]}"
    if [[ ${#SSH_PREFIX[@]} -gt 0 ]]; then
        RSYNC_SSH="sshpass -p $VPS_PASS ssh ${SSH_OPTS[*]}"
    fi
    rsync -az -s -e "$RSYNC_SSH" \
        --exclude '.git' --exclude 'build' --exclude 'sysroot' --exclude 'mios-bin' \
        --exclude 'third_party' --exclude 'MiOS.iso' --exclude '*.cpio.gz' \
        --exclude 'initramfs/dev' --exclude 'initramfs/tmp' \
        "$TARGET:$VPS_DIR/" "$ROOT/"
else
    echo "[sync] de vuelta (tar): trayendo del VPS..."
    # -h: enlaces simbolicos como fichero real; --hard-dereference: los hard links
    # como copias; --ignore-failed-read: si algo es ilegible (ficheros de root), sigue.
    "${SSH_PREFIX[@]}" ssh "${SSH_OPTS[@]}" "$TARGET" \
        "cd '$VPS_DIR' && tar -chzf - --hard-dereference --ignore-failed-read \
            --exclude='./.git' --exclude='./build' --exclude='./sysroot' --exclude='./mios-bin' \
            --exclude='./third_party' --exclude='./MiOS.iso' --exclude='./*.cpio.gz' \
            --exclude='./initramfs/dev' --exclude='./initramfs/tmp' ." \
        | tar -xzf - -C "$ROOT"
fi

echo "[ok] Traido del VPS: los dos lados quedan iguales (subido y traido)"
echo "[recordatorio] Compila y prueba en el VPS; las pruebas dentro del SO las hace el usuario."
