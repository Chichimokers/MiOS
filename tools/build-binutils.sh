#!/usr/bin/env bash
# build-binutils.sh - Compila 'as' y 'ld' de binutils PARA MiOS (Fase 7).
#
# La idea: el ABI de syscalls de MiOS es el de Linux x86_64, asi que mios-gcc
# produce binarios que corren tanto dentro de MiOS como en el propio VPS (ver
# tools/mios-host-run.sh).  Por eso binutils se configura en modo NATIVO
# (--build/--host/--target = x86_64-linux-gnu) con CC=mios-gcc: su 'configure'
# compila y EJECUTA cientos de programas de prueba, y 'make' construye bfd,
# libiberty, opcodes, gas y ld con nuestra libc.
#
# Lo que sale de aqui son dos ejecutables dinamicos de MiOS:
#
#   <out>/bin/as   ensamblador GNU (x86_64, sintaxis AT&T)
#   <out>/bin/ld   enlazador GNU (ELF x86_64, --dynamic-linker, -l...)
#
# y build.sh los copia al initramfs (/bin/as y /bin/ld).  Son los que GCC
# necesitara en la Fase 9: gcc invoca 'as' para ensamblar y 'ld' para enlazar.
#
# Detalle importante: binutils RECHAZA rutas con espacios (y el repo del VPS se
# llama "OS Nuevo test"), asi que el trabajo se hace en un espejo sin espacios
# ($WORK, por defecto ~/mios-btl): alli vive el arbol de build, el prefijo de
# instalacion y un envoltorio 'cc' que llama a mios-gcc.
#
# Requisitos en el VPS:
#   sudo apt install make        (binutils trae sus Makefile ya generados)
#   sudo bash tools/mios-host-run.sh
#
# Uso:  bash tools/build-binutils.sh              (configura, compila, instala)
#       bash tools/build-binutils.sh --no-install (no toca el initramfs)
#       bash tools/build-binutils.sh --reconfigure  (repite configure: hay que
#                                      hacerlo cuando la libc gana funciones que
#                                      configure probo antes de que existieran,
#                                      como realpath -> HAVE_REALPATH)
#       bash tools/build-binutils.sh --clean        (borra el arbol y empieza)
set -Eeuo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="$ROOT/third_party/binutils"
INITRAMFS="$ROOT/initramfs"
SYSROOT="$ROOT/sysroot"

# Espejo sin espacios (fuera del repo, no lo toca sync-vps).
WORK="${MIOS_BINUTILS_WORK:-$HOME/mios-btl}"
WSRC="$WORK/src"
BUILD="$WORK/build"
OUT="$WORK/inst"
CCWRAP="$WORK/cc"
CLOG="$BUILD/configure.log"
MLOG="$BUILD/make.log"

NO_INSTALL=0
RECONFIGURE=0
CLEAN=0
for arg in "$@"; do
    case "$arg" in
    --no-install) NO_INSTALL=1 ;;
    --reconfigure) RECONFIGURE=1 ;;
    --clean) CLEAN=1 ;;
    *) echo "opcion desconocida: $arg" >&2; exit 2 ;;
    esac
done

die() { echo "build-binutils: $*" >&2; exit 1; }

[[ -f "$SRC/gas/as.c" ]] || die "faltan las fuentes.  Ejecuta: bash tools/fetch-binutils.sh"
[[ -f "$ROOT/mios-gcc" ]] || die "falta mios-gcc"
[[ -d "$SYSROOT/usr/lib" ]] || die "falta el sysroot.  Ejecuta ./build.sh primero."

command -v make >/dev/null 2>&1 || die "falta 'make' en el VPS (sudo apt install make)"

# 'configure' comprueba cosas ejecutando programas: el VPS tiene que poder
# ejecutar lo que produce mios-gcc.
if [[ ! -e /lib/ld-mios.so.1 || ! -e /lib/libmios.so.6 ]]; then
    die "el VPS no puede ejecutar binarios de MiOS todavia.
      Ejecuta una vez:  sudo bash tools/mios-host-run.sh"
fi

JOBS="$(nproc 2>/dev/null || echo 2)"
[[ "$JOBS" -gt 4 ]] && JOBS=4

export LC_ALL=C

if [[ "$CLEAN" == "1" ]]; then
    echo "[binutils] --clean: borrando $BUILD"
    rm -rf "$BUILD"
fi
mkdir -p "$WORK" "$WSRC" "$BUILD" "$OUT"

# ------------------------------------------------------------------
# 0. espejo sin espacios + envoltorio del compilador
# ------------------------------------------------------------------
ln -sfn "$SRC" "$WSRC"

# El repo puede estar en una ruta con espacios ("OS Nuevo test"), asi que a
# binutils se le da un 'cc' sin espacios que llama a mios-gcc.  Tambien vale
# para que 'make' no parta la ruta en trozos al invocar $(CC).
cat > "$CCWRAP" <<EOF
#!/bin/sh
# generado por tools/build-binutils.sh
exec bash "$ROOT/mios-gcc" "\$@"
EOF
chmod +x "$CCWRAP"

# ------------------------------------------------------------------
# 1. configure (una sola vez)
# ------------------------------------------------------------------
if [[ ! -f "$BUILD/Makefile" || "$RECONFIGURE" == "1" ]]; then
    echo "[binutils] configurando (log: $CLOG)..."
    if [[ "$RECONFIGURE" == "1" ]]; then
        # binutils guarda los resultados de sus pruebas en config.cache (el
        # configure de arriba se lo pasa a los subdirectorios): si no se borra,
        # reconfigurar no vuelve a probar nada y una funcion nueva de la libc
        # (p. ej. realpath) se queda como "no" para siempre.
        find "$BUILD" -name config.cache -delete 2>/dev/null || true
        echo "[binutils] config.cache borrados (se vuelven a probar las funciones)"
    fi
    (
        cd "$BUILD"
        CC="$CCWRAP" \
        CFLAGS="-O2" \
        "$WSRC/configure" \
            --build=x86_64-linux-gnu \
            --host=x86_64-linux-gnu \
            --target=x86_64-linux-gnu \
            --prefix="$OUT" \
            --disable-nls \
            --disable-werror \
            --disable-shared \
            --disable-rpath \
            --disable-gold \
            --disable-gprofng \
            --disable-sim \
            --without-zlib
    ) > "$CLOG" 2>&1 || {
        echo "[binutils] configure ha fallado; ultimas lineas:" >&2
        tail -n 40 "$CLOG" >&2
        exit 1
    }
    echo "[binutils] configure OK"
    if [[ "$RECONFIGURE" == "1" ]]; then
        echo "[binutils] funciones que la libc ofrece ahora (HAVE_*):"
        {
            grep -hE '^#define HAVE_(REALPATH|CANONICALIZE_FILE_NAME|UTIME|MSYNC|WCSLEN|SETLOCALE) ' \
                "$BUILD"/config.h "$BUILD"/*/config.h 2>/dev/null | sort -u | sed 's/^/  /'
            echo "  (los subdirectorios se reconfiguran al empezar el make)"
        } || true
    fi
else
    echo "[binutils] configure ya hecho ($BUILD)"
fi

# ------------------------------------------------------------------
# 2. make: solo lo necesario (bfd, libiberty, opcodes, gas, ld)
# ------------------------------------------------------------------
echo "[binutils] compilando as y ld con make -j$JOBS (log: $MLOG)..."
( cd "$BUILD" && make -j"$JOBS" MAKEINFO=true all-gas all-ld ) > "$MLOG" 2>&1 || {
    echo "[binutils] el build ha fallado. Errores:" >&2
    grep -n -E 'error:|Error [0-9]+|undefined reference|No such file' "$MLOG" | tail -n 40 >&2
    echo "--- ultimas lineas ---" >&2
    tail -n 40 "$MLOG" >&2
    exit 1
}
echo "[binutils] compilado OK"

# ------------------------------------------------------------------
# 3. instalacion en $OUT (as, ld y las cabeceras/bibliotecas de bfd)
# ------------------------------------------------------------------
# Siempre: si no se refresca, la prueba de abajo y el initramfs acaban usando
# un as/ld viejo (el 'install' solo copia ficheros, es barato).
echo "[binutils] instalando en $OUT..."
( cd "$BUILD" && make MAKEINFO=true install-gas install-ld ) > "$BUILD/install.log" 2>&1 || {
    tail -n 30 "$BUILD/install.log" >&2
    exit 1
}

ls -l "$OUT/bin/as" "$OUT/bin/ld"

echo
echo "[binutils] version que reportan (ya ejecutandose en el VPS):"
"$OUT/bin/as" --version | head -n 1
"$OUT/bin/ld" --version | head -n 1

# ------------------------------------------------------------------
# 4. Prueba real: .s -> .o con nuestro as -> ejecutable con nuestro ld
# ------------------------------------------------------------------
echo
echo "[binutils] prueba real: as + ld, sin gcc de por medio..."
TMP="$WORK/prueba"
mkdir -p "$TMP"
"$OUT/bin/as" "$ROOT/examples/asld-hola.s" -o "$TMP/hola.o"
"$OUT/bin/ld" -o "$TMP/hola" \
    --dynamic-linker=/lib/ld-mios.so.1 \
    -z noexecstack \
    "$SYSROOT/usr/lib/crt1.o" \
    "$TMP/hola.o" \
    "$SYSROOT/usr/lib/libmios.so"
echo "  ejecutable: $(file -b "$TMP/hola" | cut -c1-60)"
readelf -d "$TMP/hola" | grep -E 'NEEDED' || true
"$TMP/hola"
echo "  el programa devolvio $? (0 = bien)"

if [[ "$NO_INSTALL" == "1" ]]; then
    echo
    echo "[binutils] --no-install: no se toca el initramfs."
    exit 0
fi

# ------------------------------------------------------------------
# 5. Instalar en el initramfs
# ------------------------------------------------------------------
echo
echo "[binutils] instalando en el initramfs..."
mkdir -p "$INITRAMFS/bin" "$INITRAMFS/usr/src"
cp -f "$OUT/bin/as" "$INITRAMFS/bin/as"
cp -f "$OUT/bin/ld" "$INITRAMFS/bin/ld"
cp -f "$ROOT/examples/asld-hola.s" "$INITRAMFS/usr/src/asld-hola.s"
# libmios.so (sin version) para que 'ld -lmios' encuentre la biblioteca.
ln -sf libmios.so.6 "$INITRAMFS/lib/libmios.so"
ls -l "$INITRAMFS/bin/as" "$INITRAMFS/bin/ld"
echo "[binutils] OK"
