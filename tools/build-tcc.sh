#!/usr/bin/env bash
# build-tcc.sh - Compila el tcc de MiOS (Fase 5) y lo instala en el initramfs.
#
# tcc se compila con mios-gcc en *una sola unidad de traduccion*: tcc.c incluye
# libtcc.c (ONE_SOURCE=1, el valor por defecto de tcc), asi que basta un fichero.
#
# Lo que se instala en el initramfs:
#
#   bin/tcc                  el compilador
#   lib/tcc/include/*.h      cabeceras privadas de tcc (stdarg, stddef, ...)
#   lib/tcc/libtcc1.a        ayudas de ejecucion que tcc enlaza en los programas
#   usr/include/*.h          cabeceras publicas de MiOS (las del sysroot)
#   usr/lib/crt1.o           arranque de los programas que produzca tcc
#   usr/lib/crti.o crtn.o    vacios (tcc los pide; MiOS no los necesita)
#   lib/libc.so libm.so      enlaces a libmios.so.6 (tcc enlaza -lc por defecto)
#
# La adaptacion de tcc a MiOS son dos cosas: third_party/tcc/config.h, que genera
# este script (rutas, interprete ELF y enlazado estatico), y un parche de tres
# lineas al final de main() de tcc.c (tools/tcc-mios-info.inc) para que diga
# claramente si la compilacion ha ido bien o mal.  El parche se aplica a una
# *copia* de tcc.c en build/, asi que las fuentes de third_party/ quedan intactas.
#
# Uso:  bash tools/build-tcc.sh            (compila e instala)
#       bash tools/build-tcc.sh --no-install
set -Eeuo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="$ROOT/third_party/tcc"
OUT="$ROOT/build/tcc"
INITRAMFS="$ROOT/initramfs"
SYSROOT="$ROOT/sysroot"
PARCHE="$ROOT/tools/tcc-mios-info.inc"
NO_INSTALL=0

[[ "${1:-}" == "--no-install" ]] && NO_INSTALL=1

# mios-gcc puede haber perdido el bit de ejecucion al sincronizar (Windows no
# guarda permisos): se invoca con bash, igual que hace build.sh con sus scripts.
MIOS_GCC=(bash "$ROOT/mios-gcc")

if [[ ! -f "$SRC/tcc.h" ]]; then
    echo "build-tcc: faltan las fuentes de tcc.  Ejecuta: bash tools/fetch-tcc.sh" >&2
    exit 1
fi

mkdir -p "$OUT"

# ------------------------------------------------------------------
# 1. config.h: lo unico que hay que adaptar de tcc para MiOS
# ------------------------------------------------------------------
echo "[tcc] generando config.h..."

cat > "$SRC/config.h" <<'EOF'
/* config.h - generado por tools/build-tcc.sh; NO EDITAR A MANO.
 *
 * Es la adaptacion de tcc a MiOS: en vez de pasar decenas de -D por linea de
 * ordenes, las rutas y el interprete ELF se fijan aqui (tcc.h incluye este
 * fichero lo primero y respeta lo ya definido).
 */

#define TCC_VERSION "0.9.27"
#define TCC_TARGET_X86_64 1

/* Directorio propio de tcc: cabeceras privadas y libtcc1.a. */
#define CONFIG_TCCDIR "/lib/tcc"

/* libtcc1.a se busca dentro de CONFIG_TCCDIR. */
#define TCC_LIBTCC1 "libtcc1.a"

/* El interprete de los ejecutables que produce tcc es el cargador de MiOS. */
#define CONFIG_TCC_ELFINTERP "/lib/ld-mios.so.1"

/* Arranque (crt1.o, crti.o, crtn.o) y bibliotecas. */
#define CONFIG_TCC_CRTPREFIX "/usr/lib"
#define CONFIG_TCC_LIBPATHS "/usr/lib:/lib:/usr/local/lib"

/* Cabeceras: las privadas de tcc, luego las publicas de MiOS. */
#define CONFIG_TCC_SYSINCLUDEPATHS "/lib/tcc/include:/usr/local/include:/usr/include"

/* En MiOS no hay dlopen: libtcc va enlazada estaticamente dentro de tcc.
 * CONFIG_TCCBOOT quita ademas el seguimiento de pila (backtrace) y la
 * comprobacion de limites (bcheck), que necesitarian la libc de glibc. */
#define CONFIG_TCC_STATIC 1
#define CONFIG_TCCBOOT 1
EOF

# ------------------------------------------------------------------
# 2. Compilar tcc (con el parche de MiOS)
# ------------------------------------------------------------------
echo "[tcc] aplicando el parche de MiOS al final de main() de tcc.c..."

if [[ ! -f "$PARCHE" ]]; then
    echo "build-tcc: falta $PARCHE" >&2
    exit 1
fi

# La copia parcheada vive en build/ (con -I"$SRC" los #include "..." de tcc.c
# siguen encontrando libtcc.c y lo demas).  El punto de insercion es la unica
# llamada a tcc_delete(s); al final de main(), donde `ret`, `s->outfile` y
# `s->output_type` todavia valen.
awk -v parche="$PARCHE" '
    !hecho && index($0, "tcc_delete(s);") > 0 {
        while ((getline linea < parche) > 0) {
            print linea
        }
        close(parche)
        hecho = 1
    }
    { print }
' "$SRC/tcc.c" > "$OUT/tcc.c"

if ! grep -q 'compilacion correcta' "$OUT/tcc.c"; then
    echo "build-tcc: no se ha podido parchear tcc.c (busca 'tcc_delete(s);')" >&2
    exit 1
fi

echo "[tcc] compilando tcc.c (mios-gcc -c)..."
"${MIOS_GCC[@]}" -c "$OUT/tcc.c" -o "$OUT/tcc.o" -I"$SRC"

echo "[tcc] enlazando tcc..."
"${MIOS_GCC[@]}" "$OUT/tcc.o" -o "$OUT/tcc"
ls -l "$OUT/tcc"

# ------------------------------------------------------------------
# 3. libtcc1.a (ayudas que tcc enlaza en los programas que compila)
# ------------------------------------------------------------------
# Los objetos son los del x86_64 de Linux (lib/Makefile: OBJ-x86_64), menos
# bcheck.o: es solo para el modo de comprobacion de limites (-b), que esta
# desactivado al compilar tcc con CONFIG_TCCBOOT.
#
# va_list.o NO es opcional: la cabecera stdarg.h que tcc usa en x86_64 manda
# va_start/va_arg a __va_start/__va_arg, que viven aqui.  Sin el, cualquier
# programa con funciones de argumentos variables falla al enlazar con
# "undefined symbol '__va_start'" (le paso al propio tcc al compilarse).
echo "[tcc] construyendo libtcc1.a..."
"${MIOS_GCC[@]}" -c "$SRC/lib/libtcc1.c"        -o "$OUT/libtcc1.o"        -I"$SRC"
"${MIOS_GCC[@]}" -c "$SRC/lib/va_list.c"        -o "$OUT/va_list.o"        -I"$SRC"
"${MIOS_GCC[@]}" -c "$SRC/lib/alloca86_64.S"    -o "$OUT/alloca86_64.o"    -I"$SRC" || true
"${MIOS_GCC[@]}" -c "$SRC/lib/alloca86_64-bt.S" -o "$OUT/alloca86_64-bt.o" -I"$SRC" || true
rm -f "$OUT/libtcc1.a"
ar rcs "$OUT/libtcc1.a" "$OUT/libtcc1.o" "$OUT/va_list.o"
for f in alloca86_64.o alloca86_64-bt.o; do
    [[ -f "$OUT/$f" ]] && ar rcs "$OUT/libtcc1.a" "$OUT/$f"
done
ls -l "$OUT/libtcc1.a"
ar t "$OUT/libtcc1.a" | tr '\n' ' '
echo

if [[ "$NO_INSTALL" == "1" ]]; then
    echo "[tcc] --no-install: no se toca el initramfs."
    exit 0
fi

# ------------------------------------------------------------------
# 4. Instalar en el initramfs (lo que vera MiOS en el arranque)
# ------------------------------------------------------------------
echo "[tcc] instalando en el initramfs..."

mkdir -p "$INITRAMFS/bin" "$INITRAMFS/lib/tcc/include" "$INITRAMFS/usr/include" \
         "$INITRAMFS/usr/lib" "$INITRAMFS/tmp"

cp -f "$OUT/tcc"                      "$INITRAMFS/bin/tcc"
cp -f "$OUT/libtcc1.a"                "$INITRAMFS/lib/tcc/libtcc1.a"
cp -a "$SRC"/include/.                "$INITRAMFS/lib/tcc/include/"
cp -a "$ROOT"/include/.               "$INITRAMFS/usr/include/"

# Un .c de ejemplo para poder probar tcc dentro de MiOS sin teclear nada.
mkdir -p "$INITRAMFS/usr/src"
cp -f "$ROOT/examples/tcc-hola.c"     "$INITRAMFS/usr/src/hola.c"

# Las fuentes de tcc, dentro de MiOS (Fase 6): con ellas tcc puede compilarse a
# si mismo sin salir del sistema.  Bastan los .c/.h de la raiz (que es de donde
# tira ONE_SOURCE: tcc.c -> libtcc.c -> los demas), mas lib/ e include/.
echo "[tcc] instalando las fuentes de tcc en /usr/src/tcc..."
mkdir -p "$INITRAMFS/usr/src/tcc/lib" "$INITRAMFS/usr/src/tcc/include"
for f in "$SRC"/*.c "$SRC"/*.h "$SRC"/*.def; do
    [[ -e "$f" ]] && cp -f "$f" "$INITRAMFS/usr/src/tcc/"
done
for f in "$SRC"/lib/* "$SRC"/include/*; do
    [[ -f "$f" ]] && cp -f "$f" "$INITRAMFS/usr/src/tcc/$(basename "$(dirname "$f")")/"
done

# El tcc.c que va dentro de MiOS es el parcheado, no el de upstream: asi el tcc
# que se compile a si mismo (Fase 6) tambien dice el resultado de la compilacion.
cp -f "$OUT/tcc.c" "$INITRAMFS/usr/src/tcc/tcc.c"
du -sh "$INITRAMFS/usr/src/tcc"

# crt1.o (el de verdad) y crti.o/crtn.o vacios.
cp -f "$SYSROOT/usr/lib/crt1.o"       "$INITRAMFS/usr/lib/crt1.o"
: > "$OUT/empty.c"
gcc -c -x c "$OUT/empty.c" -o "$INITRAMFS/usr/lib/crti.o"
cp -f "$INITRAMFS/usr/lib/crti.o"     "$INITRAMFS/usr/lib/crtn.o"

# tcc enlaza "-lc" por defecto: en MiOS la libc es libmios.so.6.
ln -sf libmios.so.6 "$INITRAMFS/lib/libc.so"
ln -sf libmios.so.6 "$INITRAMFS/lib/libm.so"

echo "[tcc] initramfs:"
ls -l "$INITRAMFS/bin/tcc" "$INITRAMFS/lib/tcc/libtcc1.a" "$INITRAMFS/usr/lib"
ls "$INITRAMFS/usr/include" | tr '\n' ' '
echo
echo "[tcc] OK"
