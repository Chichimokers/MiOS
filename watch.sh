
#!/usr/bin/env bash
set -Eeuo pipefail

# Directorio donde se encuentra este script
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

# Verificar dependencias
if ! command -v inotifywait >/dev/null 2>&1; then
    echo "ERROR: no se encuentra inotifywait."
    echo "Instala con: sudo apt install inotify-tools"
    exit 1
fi

if [[ ! -d "$ROOT/src" ]]; then
    echo "ERROR: no existe el directorio src/"
    exit 1
fi

if [[ ! -x "$ROOT/build.sh" ]]; then
    echo "ERROR: build.sh no existe o no es ejecutable."
    echo "Ejecuta: chmod +x '$ROOT/build.sh'"
    exit 1
fi

echo "=================================="
echo "         MiOS AUTO-BUILD"
echo "=================================="
echo "Proyecto: $ROOT"
echo "Vigilando: $ROOT/src"
echo "Pulsa Ctrl+C para detener."
echo

while true; do
    # Esperar el siguiente cambio
    if changed="$(
        inotifywait -q -e close_write,moved_to,create \
            --format '%w%f' "$ROOT/src"
    )"; then
        case "$changed" in
            *.c|*.cpp|*.h|*.hpp)
                echo
                echo "Cambio detectado: $changed"
                echo "Reconstruyendo MiOS..."

                if "$ROOT/build.sh"; then
                    echo
                    echo "ISO actualizada correctamente."
                else
                    echo
                    echo "ERROR: falló la compilación."
                    echo "Corrige el código y guarda de nuevo."
                fi
                ;;
        esac
    else
        echo "Error al vigilar archivos; reintentando..."
        sleep 2
    fi
done
