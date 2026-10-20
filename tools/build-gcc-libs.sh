#!/usr/bin/env bash
# build-gcc-libs.sh - GMP, MPFR y MPC para MiOS (Fase 8).
#
# Son las tres bibliotecas aritmeticas que GCC necesita (mpfr y mpc por encima
# de gmp).  Igual que con binutils, se construyen con 'mios-gcc': el ABI de
# MiOS es el de Linux x86_64, asi que los 'configure' pueden compilar y
# EJECUTAR sus programas de prueba en el VPS (tools/mios-host-run.sh) y las
# bibliotecas resultantes se cargan igual en MiOS desde /lib.
#
# Lo que sale de aqui (prefijo $WORK/inst, por defecto ~/mios-gcl/inst):
#
#   lib/libgmp.so.10   lib/libmpfr.so.6   lib/libmpc.so.3
#   include/gmp.h      include/mpfr.h     include/mpc.h
#
# y, si no se pasa --no-install, se copian tambien al sysroot y al initramfs
# (/lib y /usr/include), que es lo que vera GCC dentro de MiOS.
#
# Detalle: MPFR se construye con --disable-thread-safe.  MPFR 4.x usa TLS
# (__thread) para sus caches y el cargador de MiOS todavia no implementa TLS
# dentro de una biblioteca compartida (R_X86_64_TPOFF64 / __tls_get_addr).  MiOS
# es mono-hilo, asi que las caches globales son equivalentes.
#
# Requisitos en el VPS: make, m4 y 'sudo bash tools/mios-host-run.sh'.
#
# Uso:  bash tools/build-gcc-libs.sh
#       bash tools/build-gcc-libs.sh --no-install
#       bash tools/build-gcc-libs.sh --reconfigure   (repite los configure)
#       bash tools/build-gcc-libs.sh --clean         (borra el arbol y empieza)
set -Eeuo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="$ROOT/third_party"
SYSROOT="$ROOT/sysroot"
INITRAMFS="$ROOT/initramfs"

# Espejo sin espacios (fuera del repo, no lo toca sync-vps).
WORK="${MIOS_GCL_WORK:-$HOME/mios-gcl}"
WSRC="$WORK/src"
BUILDROOT="$WORK/build"
OUT="$WORK/inst"
CCWRAP="$WORK/cc"

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

die() { echo "build-gcc-libs: $*" >&2; exit 1; }

[[ -f "$SRC/gmp/configure"   ]] || die "faltan las fuentes de gmp.  Ejecuta: bash tools/fetch-gcc-libs.sh"
[[ -f "$SRC/mpfr/configure"  ]] || die "faltan las fuentes de mpfr.  Ejecuta: bash tools/fetch-gcc-libs.sh"
[[ -f "$SRC/mpc/configure"   ]] || die "faltan las fuentes de mpc.  Ejecuta: bash tools/fetch-gcc-libs.sh"
[[ -f "$ROOT/mios-gcc"       ]] || die "falta mios-gcc"
[[ -d "$SYSROOT/usr/lib"     ]] || die "falta el sysroot.  Ejecuta ./build.sh primero."

command -v make >/dev/null 2>&1 || die "falta 'make' en el VPS (sudo apt install make)"

if [[ ! -e /lib/ld-mios.so.1 || ! -e /lib/libmios.so.6 ]]; then
    die "el VPS no puede ejecutar binarios de MiOS todavia.
      Ejecuta una vez:  sudo bash tools/mios-host-run.sh"
fi

# Las cabeceras que ve el compilador son las del SYSROOT, no las de include/:
# si se ha tocado include/, el sysroot esta viejo y aqui se compilaria con la
# version antigua (paso con _STDIO_H: GMP no reconocia stdio.h y escondia sus
# prototipos con FILE).
if [[ -f "$SYSROOT/usr/include/stdio.h" && "$ROOT/include/stdio.h" -nt "$SYSROOT/usr/include/stdio.h" ]]; then
    echo "[gcc-libs] AVISO: el sysroot tiene cabeceras mas viejas que include/." >&2
    echo "[gcc-libs]        Ejecuta 'bash mk-sysroot.sh' (o ./build.sh) y repite." >&2
fi

JOBS="$(nproc 2>/dev/null || echo 2)"
[[ "$JOBS" -gt 4 ]] && JOBS=4

export LC_ALL=C

if [[ "$CLEAN" == "1" ]]; then
    echo "[gcc-libs] --clean: borrando $BUILDROOT"
    rm -rf "$BUILDROOT"
fi
mkdir -p "$WORK" "$BUILDROOT" "$OUT"

# ------------------------------------------------------------------
# 0. espejo sin espacios + envoltorio del compilador
# ------------------------------------------------------------------
mkdir -p "$WSRC"
ln -sfn "$SRC/gmp"  "$WSRC/gmp"
ln -sfn "$SRC/mpfr" "$WSRC/mpfr"
ln -sfn "$SRC/mpc"  "$WSRC/mpc"

cat > "$CCWRAP" <<EOF
#!/bin/sh
# generado por tools/build-gcc-libs.sh
exec bash "$ROOT/mios-gcc" "\$@"
EOF
chmod +x "$CCWRAP"

# ------------------------------------------------------------------
# 1. construir un paquete: construir <nombre> <args de configure...>
# ------------------------------------------------------------------
construir() {
    local pkg="$1"; shift
    local bdir="$BUILDROOT/$pkg"
    local clog="$bdir.log" tlog="$bdir.make.log"

    if [[ ! -f "$bdir/Makefile" || "$RECONFIGURE" == "1" ]]; then
        echo "[gcc-libs] configurando $pkg (log: $clog)..."
        if [[ "$RECONFIGURE" == "1" ]]; then
            find "$bdir" -name config.cache -delete 2>/dev/null || true
        fi
        mkdir -p "$bdir"
        (
            cd "$bdir"
            CC="$CCWRAP" \
            CFLAGS="-O2" \
            CPPFLAGS="-I$OUT/include" \
            LDFLAGS="-L$OUT/lib" \
            "$WSRC/$pkg/configure" \
                --build=x86_64-pc-linux-gnu \
                --host=x86_64-pc-linux-gnu \
                --prefix="$OUT" \
                "$@"
        ) > "$clog" 2>&1 || {
            echo "[gcc-libs] configure de $pkg ha fallado; ultimas lineas:" >&2
            tail -n 40 "$clog" >&2
            exit 1
        }
        echo "[gcc-libs] configure de $pkg OK"
    else
        echo "[gcc-libs] $pkg ya configurado ($bdir)"
    fi

    echo "[gcc-libs] compilando $pkg con make -j$JOBS (log: $tlog)..."
    ( cd "$bdir" && make -j"$JOBS" ) > "$tlog" 2>&1 || {
        echo "[gcc-libs] el make de $pkg ha fallado. Errores:" >&2
        grep -n -E 'error:|Error [0-9]+|undefined reference|No such file' "$tlog" | tail -n 30 >&2
        echo "--- ultimas lineas ---" >&2
        tail -n 30 "$tlog" >&2
        exit 1
    }

    echo "[gcc-libs] instalando $pkg en $OUT..."
    ( cd "$bdir" && make install ) >> "$tlog" 2>&1 || {
        tail -n 30 "$tlog" >&2
        exit 1
    }
    echo "[gcc-libs] $pkg OK"
}

# ------------------------------------------------------------------
# 2. GMP, MPFR y MPC (en este orden: cada uno usa al anterior)
# ------------------------------------------------------------------
echo "[gcc-libs] construyendo GMP, MPFR y MPC para MiOS..."
echo

construir gmp \
    --enable-shared --enable-static \
    --disable-cxx

construir mpfr \
    --with-gmp="$OUT" \
    --enable-shared --enable-static \
    --disable-thread-safe --disable-float128 --disable-decimal-float

construir mpc \
    --with-gmp="$OUT" --with-mpfr="$OUT" \
    --enable-shared --enable-static

echo
echo "[gcc-libs] lo instalado:"
ls -l "$OUT"/lib/libgmp.so.* "$OUT"/lib/libmpfr.so.* "$OUT"/lib/libmpc.so.* 2>/dev/null | head -n 12
ls -l "$OUT"/include/gmp.h "$OUT"/include/mpfr.h "$OUT"/include/mpc.h

# ------------------------------------------------------------------
# 2b. Simbolos sin resolver: una biblioteca compartida se enlaza aunque le
#     falten simbolos (el error saldria al cargarla dentro de MiOS), asi que se
#     comprueba aqui, contra lo que exporta libmios.
# ------------------------------------------------------------------
echo
echo "[gcc-libs] simbolos sin resolver (contando lo que definen libmios y las tres):"
defs="$(mktemp)"
{
    nm -D --defined-only "$SYSROOT/lib/libmios.so.6" 2>/dev/null
    for s in "$OUT"/lib/libgmp.so.*.* "$OUT"/lib/libmpfr.so.*.* "$OUT"/lib/libmpc.so.*.*; do
        [[ -e "$s" ]] && nm -D --defined-only "$s" 2>/dev/null
    done
} | awk 'NF >= 3 { print $3 }' | sort -u > "$defs"
for par in "gmp:libgmp" "mpfr:libmpfr" "mpc:libmpc"; do
    pkg="${par%%:*}"; base="${par#*:}"
    so="$(ls "$OUT/lib/$base.so."*.* 2>/dev/null | head -n 1)"
    [[ -n "$so" ]] || continue
    faltan="$(nm -D --undefined-only "$so" 2>/dev/null | awk 'NF >= 2 { print $2 }' \
              | sort -u | grep -v -x -F -f "$defs" || true)"
    if [[ -z "$faltan" ]]; then
        echo "  $pkg: todo resuelto"
    else
        echo "  $pkg: sin resolver -> $(echo "$faltan" | tr '\n' ' ')"
    fi
done
rm -f "$defs"

# ------------------------------------------------------------------
# 3. Prueba real: calculo con las tres, ejecutado en el VPS
# ------------------------------------------------------------------
echo
echo "[gcc-libs] prueba real (examples/gmptest.c) en el VPS..."
TESTBIN="$WORK/gmptest"
"$ROOT/mios-gcc" "$ROOT/examples/gmptest.c" -o "$TESTBIN" \
    -I"$OUT/include" -L"$OUT/lib" -lmpc -lmpfr -lgmp
readelf -d "$TESTBIN" | grep -E 'NEEDED' | sed 's/^/  /'
LD_LIBRARY_PATH="$OUT/lib" "$TESTBIN"
rc=$?
echo "  el programa devolvio $rc (0 = bien)"
[[ "$rc" == "0" ]] || { echo "[gcc-libs] la prueba ha fallado" >&2; exit 1; }

# ------------------------------------------------------------------
# 4. La misma prueba dentro del chroot del sysroot (solo libmios + cargador)
# ------------------------------------------------------------------
echo
echo "[gcc-libs] prueba dentro del chroot del sysroot..."
CHROOT_TMP="$(mktemp -d)"
cp -f "$OUT"/lib/libgmp.so.*  "$CHROOT_TMP/" 2>/dev/null || true
cp -f "$OUT"/lib/libmpfr.so.* "$CHROOT_TMP/" 2>/dev/null || true
cp -f "$OUT"/lib/libmpc.so.*  "$CHROOT_TMP/" 2>/dev/null || true
cp -f "$TESTBIN" "$CHROOT_TMP/gmptest"
chmod -R a+rX "$CHROOT_TMP"

echo 123 | sudo -S -p '' cp -f "$CHROOT_TMP"/*.so.* "$SYSROOT/lib/" 2>/dev/null || true
echo 123 | sudo -S -p '' cp -f "$CHROOT_TMP/gmptest" "$SYSROOT/gmptest"
echo 123 | sudo -S -p '' chroot "$SYSROOT" /gmptest
rc=$?
echo "  el chroot devolvio $rc (0 = bien)"
echo 123 | sudo -S -p '' rm -f "$SYSROOT/gmptest"
echo 123 | sudo -S -p '' sh -c "rm -f $SYSROOT/lib/libgmp.so.* $SYSROOT/lib/libmpfr.so.* $SYSROOT/lib/libmpc.so.*"
rm -rf "$CHROOT_TMP"
[[ "$rc" == "0" ]] || { echo "[gcc-libs] la prueba en el chroot ha fallado" >&2; exit 1; }

if [[ "$NO_INSTALL" == "1" ]]; then
    echo
    echo "[gcc-libs] --no-install: no se toca el sysroot ni el initramfs."
    exit 0
fi

# ------------------------------------------------------------------
# 5. Instalar en el sysroot (para enlazar en el chroot) y en el initramfs
# ------------------------------------------------------------------
echo
echo "[gcc-libs] instalando en el sysroot y en el initramfs..."
mkdir -p "$SYSROOT/usr/lib" "$SYSROOT/lib" "$SYSROOT/usr/include" \
         "$INITRAMFS/lib" "$INITRAMFS/usr/include" "$INITRAMFS/usr/src"

cp -f "$OUT"/lib/libgmp.so*  "$SYSROOT/usr/lib/"
cp -f "$OUT"/lib/libmpfr.so* "$SYSROOT/usr/lib/"
cp -f "$OUT"/lib/libmpc.so*  "$SYSROOT/usr/lib/"
cp -f "$OUT"/include/gmp.h "$OUT"/include/mpfr.h "$OUT"/include/mpc.h "$SYSROOT/usr/include/"

cp -a "$OUT"/lib/libgmp.so*  "$SYSROOT/lib/" 2>/dev/null || true
cp -a "$OUT"/lib/libmpfr.so* "$SYSROOT/lib/" 2>/dev/null || true
cp -a "$OUT"/lib/libmpc.so*  "$SYSROOT/lib/" 2>/dev/null || true

# En MiOS las bibliotecas van a /lib (el cargador busca ahi) y las cabeceras a
# /usr/include; el programa de prueba queda en /usr/src para probarlo a mano.
# Se copian con -a para conservar los enlaces (libgmp.so -> libgmp.so.10 ->
# libgmp.so.10.4.1) en vez de triplicar cada fichero.
cp -a "$OUT"/lib/libgmp.so*  "$INITRAMFS/lib/"
cp -a "$OUT"/lib/libmpfr.so* "$INITRAMFS/lib/"
cp -a "$OUT"/lib/libmpc.so*  "$INITRAMFS/lib/"
cp -f "$OUT"/include/gmp.h "$OUT"/include/mpfr.h "$OUT"/include/mpc.h "$INITRAMFS/usr/include/"
cp -f "$ROOT/examples/gmptest.c" "$INITRAMFS/usr/src/gmptest.c"
# El binario de prueba ya compilado: sirve para comprobar dentro de MiOS que las
# tres bibliotecas se cargan y funcionan (no se pone en /bin para no ensuciar el
# PATH: se ejecuta como /usr/src/gmptest).
cp -f "$WORK/gmptest" "$INITRAMFS/usr/src/gmptest"
chmod +x "$INITRAMFS/usr/src/gmptest"

ls -l "$INITRAMFS/lib/libgmp.so"* "$INITRAMFS/lib/libmpfr.so"* "$INITRAMFS/lib/libmpc.so"* | head -n 12
echo "[gcc-libs] OK"
