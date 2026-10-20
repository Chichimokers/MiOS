# Progreso — Toolchain propio de MiOS

Bitácora de avances. **Añadir una entrada al cerrar cada fase** (y también en hitos intermedios).
Formato: fecha · fase · qué se hizo · qué falta · cómo se verificó · bloqueos.

Plan completo: [`PLAN-TOOLCHAIN.md`](PLAN-TOOLCHAIN.md).

---

## Estado actual

**Fase 8 cerrada y verificada: GMP 6.2.1 + MPFR 4.1.0 + MPC 1.2.1 sobre libmios.**
*(Fases 0 a 8 cerradas y verificadas; la 9 está estudiada y espera prerrequisitos)*

| Fase | Estado |
|---|---|
| 0 — Fundación dinámica | ✅ hecho y verificado |
| 1 — Sysroot + `mios-gcc` | ✅ hecho y verificado |
| 2 — libmios C/POSIX-lite | ✅ hecho y verificado |
| 3 — libm (math) + fenv | ✅ hecho y verificado |
| 4 — Procesos + FS en MiOS | ✅ hecho y verificado |
| 5 — Portar `tcc` | ✅ hecho y verificado |
| 6 — Self-hosting C (tcc) ⭐ | ✅ hecho y verificado |
| 6b - Editor en consola (`edit`) + aviso de `tcc` | ✅ hecho y verificado |
| 7 - binutils (as, ld) | ✅ hecho y verificado |
| 8 — GMP + MPFR + MPC | ✅ hecho y verificado |
| 9 — GCC etapa C | ⬜ pendiente (necesita `g++` en el VPS y ~2 GB de disco) |
| 10 — libstdc++ + GCC C++ | ⬜ pendiente |
| 11 — Bootstrap nativo / self-hosting TOTAL | ⬜ pendiente |

Leyenda: ⬜ pendiente · 🟡 en curso · ✅ hecho · ⛔ bloqueado

> **Prerrequisitos de la Fase 9 (estudio previo, 2026-10-19).** GCC está escrito en C++: hace falta
> **`g++`** (el `cc1plus` y las cabeceras de libstdc++) en el VPS para compilar sus fuentes, más
> `texinfo`/`gperf` por si el *build* los pide (el binario `makeinfo` sí está instalado). La buena
> noticia del análisis: un `cc1` real **no enlaza libstdc++** (0 símbolos `std::` sin resolver y
> solo un par del tipo `__cxa_*`), así que basta con un **runtime C++ mínimo propio** en libmios
> (`operator new`/`delete`, `__cxa_atexit`, guardas de estáticos locales) en vez de portar toda la
> STL. Ese `cc1` usa además libisl (solo Graphite: si no hay `isl/*.h`, GCC se configura sin
> Graphite), libz y libzstd (GCC trae su propia zlib; zstd es opcional). **Espacio:** el árbol de
> GCC pide ~2 GB y al VPS le quedan ~2.4 GB (el usuario ampliará el disco antes de empezar).
> `tools/fetch-gcc.sh` ya está escrito (baja GCC 13.3.0); la descarga se cortó al aparcar la fase
> (deja `third_party/gcc-13.3.0.tar.xz` a medias, que se sobrescribe al reintentar).

---

## Registro de cambios

### 2026-10-20 - Incidencia y mejora: sync bidireccional + permisos del initramfs

- **Hecho:**
  - El **sync pasa a ser bidireccional**: `sync-vps.bat` primero sube (fuentes) y después **trae**
    del VPS los artefactos del build (nuevo `pull-vps.bat`, que también sirve como solo-traer). Así
    Windows deja de quedarse con una copia vieja de lo compilado y los dos lados convergen en un
    solo paso.
  - El **push ya no envía artefactos generados** (`initramfs/init`, `initramfs/bin`, `initramfs/usr`,
    `initramfs/lib/tcc`, `initramfs/lib/*.so*` y `ld-mios.so.1`): se generan en el VPS, y subirlos
    desde Windows convertía sus **enlaces** (`libc.so` → `libmios.so.6`, `libgmp.so` →
    `libgmp.so.10.4.1`) en copias del fichero entero.
  - El push **devuelve el bit `+x`** a `*.sh`, `tools/*.sh` y `mios-gcc` en el VPS: el `tar` de
    Windows no lo conserva y `build.sh` fallaba con *Permission denied*.
- **Incidencia encontrada y resuelta:** la primera ISO con el sync nuevo **no arrancaba**:
  `Failed to execute /init (error -13)` + `Kernel panic - not syncing: No working init found`. La
  causa era que `initramfs/lib/ld-mios.so.1` había quedado en **0664** (el push antiguo llevó la
  copia de Windows sin `+x`, y `cp -f` conserva el modo del destino). El kernel necesita poder
  **ejecutar** el intérprete de `/init`; sin `+x`, `execve` devuelve `EACCES`. `build.sh` ahora
  **sanea los permisos** justo antes de empaquetar (`init` y `ld-mios.so.1` a 755 y los binarios de
  `initramfs/bin/` a 755), así que un modo heredado no puede volver a romper el arranque.
- **Verificación:** `./build.sh` completo (11 pasos, sin errores) en el VPS; la ISO
  `88794eb9b1b17b0609ca126ed42c1bbe` arranca en la VM *headless* hasta el prompt `MiOS>` sin
  *panic*; en el `chroot` del initramfs `gmptest` da **0 fallos** (GMP/MPFR/MPC a través de sus
  enlaces), `tcc` compila `/usr/src/hola.c` y `/bin/hola` corre; los enlaces de `initramfs/lib/`
  vuelven a ser enlaces. Comparación hash a hash de los dos árboles: **285/285 ficheros idénticos**,
  0 distintos, 0 solo en un lado (excluyendo a propósito `third_party/`, `build/`, `sysroot/`,
  `mios-bin/`, `.git/`, `MiOS.iso`, `*.cpio.gz`, `initramfs/dev` e `initramfs/tmp`).
- **Falta:** nada de la Fase 8; la Fase 9 sigue esperando el disco y `g++`.
- **Bloqueos:** ninguno.

### 2026-10-19 - Fase 8: VERIFICADA ✅ (GMP 6.2.1 + MPFR 4.1.0 + MPC 1.2.1 sobre libmios)

- **Hecho:**
  - **Las tres bibliotecas aritméticas de GCC**, construidas con `mios-gcc` y usables **como
    bibliotecas compartidas de MiOS**: `/lib/libgmp.so.10.4.1`, `/lib/libmpfr.so.6.1.0`,
    `/lib/libmpc.so.3.2.1` (+ enlaces) y sus cabeceras en `/usr/include`. `tools/fetch-gcc-libs.sh`
    baja las versiones **exactas** que GCC 13.3 pide en su `contrib/download_prerequisites`
    (6.2.1 / 4.1.0 / 1.2.1) y `tools/build-gcc-libs.sh` hace configure + make + install en un
    espejo sin espacios (`~/mios-gcl`, la misma estratagema que binutils: `configure` nativo con
    `CC=mios-gcc` que **compila y ejecuta** sus programas de prueba en el VPS). Opciones
    `--no-install`, `--reconfigure`, `--clean`.
  - **MPFR se construye `--disable-thread-safe`**, a propósito: MPFR 4.x usa TLS (`__thread`) para
    sus cachés y el cargador todavía **no** implementa TLS *dentro* de una `.so`
    (`R_X86_64_TPOFF64`/`__tls_get_addr`). MiOS es mono-hilo, así que las cachés globales son
    equivalentes, y así no hace falta tocar el cargador para esta fase. (GMP y MPC no usan TLS.)
  - **El cargador entiende `DT_GNU_HASH`** (el hueco grande de la fase): solo miraba `DT_HASH`,
    que es lo que lleva `libmios` (`--hash-style=sysv`), pero **todo lo que enlaza el `gcc` del
    VPS** (GMP, MPFR, MPC) lleva por defecto solo `--hash-style=gnu`. Sin `DT_HASH` su `nsym`
    quedaba en 0, su tabla de símbolos parecía vacía y **cada** símbolo que exportaban se
    resolvía a 0: el programa saltaba a la dirección 0 (SIGSEGV sin más pistas). Ahora se calcula
    el número de símbolos recorriendo las cadenas de la tabla GNU.
  - **`DT_NEEDED` recursivo y sin duplicados** en el cargador: una librería puede necesitar otra
    que el ejecutable no nombra (`libmpc` → `libmpfr` → `libgmp`) y la misma biblioteca puede
    aparecer en las `DT_NEEDED` de varias; se compara por `DT_SONAME`/nombre de fichero y no se
    carga dos veces. La búsqueda global ignora los símbolos `STB_LOCAL` (no se interponen).
  - **Un símbolo sin resolver ya no es un misterio**: si es global (no débil) el cargador dice
    `ld-mios: simbolo sin resolver: <nombre>` y para, en vez de escribir 0 y dejar que el programa
    salte a la dirección 0.
  - **`mios-gcc` sabe enlazar `-shared`**: sin `crt1.o`, sin `-e,_start` y sin
    `--dynamic-linker` (una `.so` no tiene `main` ni punto de entrada) y añadiendo `-fPIC`. Es lo
    que necesitan los `libtool` de GMP/MPFR/MPC (`cc -shared ...`).
  - **`<stdio.h>` define `_STDIO_H`** además de su guarda propia: hay software que *reconoce*
    `stdio.h` por el nombre de la guarda de glibc y no por `FILE` — GMP lo hace, y sin esa marca
    avisa "gmp.h doesnt recognise <stdio.h>" y **esconde todos sus prototipos con `FILE`**, con lo
    que `mpz/inp_str.c` ni compila.
  - **`examples/gmptest.c`**: la prueba de cálculo de las tres — `100!` con GMP, `pi`, `sqrt(2)` y
    `exp(sqrt 2)` con MPFR (a 200 bits, para comparar decimales exactos) y `(1+2i)*(3+4i)` con MPC.
    Si no se pasa `--no-install`, el script lo deja **compilado** en `/usr/src/gmptest` para poder
    comprobarlo dentro de MiOS, y `./build.sh` lo mete en la ISO.
  - `./build.sh` pasa a **11 pasos**: el 8 construye GMP/MPFR/MPC (`tools/build-gcc-libs.sh`), no
    es imprescindible para arrancar (si faltan fuentes avisa y sigue, como tcc y binutils).
- **Falta:**
  - TLS dentro de una `.so` (`R_X86_64_TPOFF64`/`__tls_get_addr`): sigue pendiente y sin
    necesidad por ahora (MPFR va sin él).
  - Opcional: `ar`/`nm`/`objdump`/`strip` (`make install-binutils`), que la Fase 9 agradecerá.
  - Que el usuario pruebe a mano en la consola, si quiere:
    `tcc -o /tmp/g /usr/src/gmptest.c -lgmp -lmpfr -lmpc`.
- **Verificación:**
  - **VPS:** `tools/build-gcc-libs.sh` → las tres compilan, se instalan, el informe de símbolos sin
    resolver dice "todo resuelto" en las tres y `gmptest` da **0 fallos** (100!, pi, sqrt(2),
    exp(sqrt 2), producto complejo), con `DT_NEEDED` = libmpc/libmpfr/libgmp/libmios.
  - **Chroot del sysroot** (solo `/lib/libmios.so.6` + `/lib/ld-mios.so.1`, como dentro de MiOS):
    **0 fallos**.
  - **VM (headless, leyendo `mioos.log`)** con `/etc/autorun` temporal: `runlog /usr/src/gmptest`
    → las cinco líneas `ok`, `Fase 8: 0 fallo(s)` y `runlog: ... termino con codigo 0`. Sin
    *kernel panic*. Es la prueba de que las tres bibliotecas se cargan y calculan **dentro de MiOS**.
  - **`./build.sh`** (11 pasos) completo: ISO de 23 MB con las tres bibliotecas dentro;
    md5 de la ISO de trabajo `6e36f31908fbd2333ec9541c4524d20f` (con la prueba en `autorun`) y
    `341664a01ee340685c36f49d61e56ea1` (build limpio; al reempaquetar cambia solo por las fechas
    que el `cpio` guarda de cada fichero, el contenido es el mismo).
  - **Comprobación de que no se rompió nada anterior** (los cambios del cargador de esta fase los
    usan *todos* los binarios dinámicos): con los **mismos** binarios que van en la ISO, en un
    `chroot` del initramfs y **dentro de MiOS** (VM headless, log serie): `tcc` compila
    `/usr/src/hola.c` y el programa pasa sus 6 pruebas; `tcc` **se compila a sí mismo** y el `tcc`
    resultante compila otro programa; `as`+`ld` ensamblan, enlazan y ejecutan (`as y ld de MiOS:
    hola`); `selftest` da 0 fallos. Todo con código 0 y sin *kernel panic*. ISO limpia resultante:
    `8b80241ee6897b132f366abd0717f050`.
- **Bloqueos:** ninguno.


### 2026-10-19 - Fase 7: VERIFICADA ✅ (`as` y `ld` de binutils 2.42 dentro de MiOS)

- **Hecho:**
  - **`as` y `ld` reales de GNU** (binutils 2.42), construidos con `mios-gcc` y ejecutándose tanto en
    el VPS como dentro de MiOS. Son los que GCC esperará en la Fase 9 (no un apaño con `tcc`).
    `tools/build-binutils.sh` hace el trabajo: `configure` **nativo** (`host=build=target=x86_64-linux-gnu`,
    `CC=mios-gcc`), espejo sin espacios en `~/mios-btl/` (binutils rechaza rutas con espacios),
    `make all-gas all-ld` (arrastra `bfd`, `libiberty`, `opcodes`), instalación en `/bin/as` y
    `/bin/ld` (+ `examples/asld-hola.s` → `/usr/src/`) y **prueba real**: ensamblar un `.s` con
    nuestro `as`, enlazarlo con nuestro `ld` y ejecutar el resultado. Opciones: `--no-install`,
    `--reconfigure`, `--clean`.
  - **TLS estático en el cargador** (`src/ld/ldmios.c`), el hueco de verdad que destapó binutils:
    `bfd_init()` leía variables `__thread` con `%fs` sin inicializar (`si_addr=0xfffffffffffffffc`)
    y el proceso moría en un SIGSEGV. Ahora el cargador lee `PT_TLS`, coloca el bloque del
    ejecutable **acabando justo en el puntero de hilo** (modelo *local-exec*, que es el que emite
    gcc sin `-fPIC`), pone detrás los bloques de las librerías, escribe el auto-puntero del TCB y
    carga `%fs` con `arch_prctl(ARCH_SET_FS)`.
  - **Libc que pidió binutils:** `realpath()` y `canonicalize_file_name()` (`src/lib/realpath.c`;
    sin ellas `lrealpath()` de libiberty **no tiene implementación** y devuelve basura — así se
    caía `ld` al enlazar), `pathconf()`/`fpathconf()` + `_PC_*` (`src/lib/pathconf.c`),
    `rlim_t`/`RLIM_SAVED_*` en `sys/resource.h` y los typedefs de sistema (`suseconds_t`, `id_t`,
    `blkcnt_t`, …) en `sys/types.h`.
  - **`config.cache` de binutils:** el `configure` de arriba se lo pasa a los subdirectorios, así
    que una función añadida a la libc se quedaba marcada como "no" para siempre
    (`checking for realpath... (cached) no`). `--reconfigure` ahora borra esos cachés y repite
    todas las pruebas. El script además **reinstala siempre** (antes, si `$OUT/bin/ld` ya existía,
    se probaba un binario viejo y el fallo parecía no arreglarse).
- **Falta:**
  - Opcional: `make install-binutils` para tener también `ar`/`nm`/`objdump`/`strip` en MiOS
    (útiles al portar GCC). Ahora mismo solo se instalan `as` y `ld`.
  - Opcional: TLS dentro de una `.so` (`R_X86_64_TPOFF64`/`__tls_get_addr`) — hoy no lo pide nadie.
  - Que el usuario pruebe `as`/`ld` a mano en la consola interactiva, si quiere.
- **Verificación:**
  - **VPS:** `as --version` y `ld --version` (Binutils 2.42) y la prueba real completa:
    `as examples/asld-hola.s -o hola.o` + `ld -o hola --dynamic-linker=/lib/ld-mios.so.1 -z
    noexecstack crt1.o hola.o libmios.so` + ejecutar → `as y ld de MiOS: hola` (código 0).
  - **Chroot del sysroot** (solo `/lib/libmios.so.6` + `/lib/ld-mios.so.1`, como dentro de MiOS):
    versiones, ensamblado, enlazado y ejecución, todo con código 0.
  - **VM (headless, leyendo `mioos.log`)** con `/etc/autorun` temporal:
    `runlog as --version` → 0, `runlog ld --version` → 0, `runlog as /usr/src/asld-hola.s -o
    /tmp/hola.o` → 0, `runlog ld -o /tmp/hola …` → 0, `runlog /tmp/hola` → `as y ld de MiOS: hola`
    (código 0). Sin *kernel panic*; el log serie muestra el banner, la consola `MiOS>` y los cinco
    resultados.
  - **`./build.sh`** (10 pasos) completo: ISO con `/bin/as` (1,9 MB) y `/bin/ld` (2,3 MB) dentro;
    md5 de la ISO de trabajo `b014b49353533783456883fe202fc0c4` (con la prueba en `autorun`) y
    `c22198eb914081b58e6ea6b56205dd53` (build limpio).
- **Bloqueos:** ninguno (el `chroot` de prueba deja ficheros de root en `sysroot/`, que hay que
  borrar con `sudo rm -rf sysroot` antes del siguiente `./build.sh`).


### 2026-10-18 - Fase 6b: VERIFICADA ✅ (editor de texto en consola y `tcc` que dice el resultado)

- **Hecho:**
  - **`tcc` informa del resultado.** Antes se callaba si todo iba bien y, si fallaba, dejaba los
    mensajes de error sueltos. Ahora siempre dice cómo ha ido:
    `tcc: compilacion correcta -> /tmp/hola3` o `tcc: la compilacion ha fallado`.
    El parche son tres líneas al final de `main()` de `tcc.c` (`tools/tcc-mios-info.inc`) que
    `tools/build-tcc.sh` inserta al copiar las fuentes a `build/` (las de `third_party/` no se
    tocan). El `tcc.c` que se instala en `/usr/src/tcc/` es el parcheado, así que el `tcc` que MiOS
    compila a sí mismo informa igual. Con `-E` (solo preprocesar) no dice nada de más.
  - **Editor de texto `edit`** (`userland/edit.c` → `/bin/edit`), a pantalla completa estilo
    `nano`: líneas en memoria, cursor con flechas / Inicio / Fin / AvPag / RePag, `Enter` parte la
    línea, `Retroceso` / `Supr` borran, `Tabulador` pone espacios hasta el múltiplo de 8, `Ctrl-S`
    guarda, `Ctrl-Q` (o `Ctrl-C`) sale pidiendo confirmación si hay cambios, `Ctrl-K` borra la línea
    entera, `Ctrl-L` repinta y `Ctrl-G` muestra/oculta la ayuda. Línea de estado con fichero,
    línea/columna, desplazamiento horizontal y marca de modificado. Carga ficheros con CRLF y sin
    `\n` final, y crea el fichero si no existe.
  - **Soporte de `termios` en la libc**: `include/termios.h` + `src/lib/termios.c` añaden
    `tcgetattr`, `tcsetattr`, `cfmakeraw` y `tcflush` sobre el `SYS_ioctl` que ya usaba `isatty`.
    El editor pone la consola en **modo crudo** mientras trabaja y la deja como estaba al salir.
  - **Dos pruebas automáticas sin consola interactiva** (para `runlog` / `/etc/autorun`):
    `edit --selftest` (motor de edición: cargar, insertar, partir, pegar, guardar, CRLF,
    tabuladores…) y `edit --ttytest` (ioctl de `termios` y modo crudo).
- **Falta:** nada de esta fase.
- **Verificación (VPS + VM):**
  - `build.sh` completo (9 pasos) sin avisos con `-Wall -Wextra` en `edit.c` ni en `termios.c`.
  - En el *chroot* del sysroot: `edit --selftest` → 17 `OK`, `resultado: 0 fallo(s)`.
  - En el *chroot* del initramfs: `tcc -o /tmp/hola /usr/src/hola.c` →
    `tcc: compilacion correcta -> /tmp/hola` (código 0) y el programa compilado se ejecuta; con un
    fichero que no existe → `tcc: error: file '...' not found` + `tcc: la compilacion ha fallado`
    (código 1).
  - Arranque de la ISO en la VM (log serie, con `/etc/autorun` de prueba temporal): las dos pruebas
    del editor con `0 fallo(s)` —incluido el modo crudo del tty de verdad— y los dos mensajes de
    `tcc` (éxito y fallo). ISO `9f1e0e7f8dcd9e67a4f524d48d43f324`, 21 198 848 B.
  - La prueba interactiva del editor (abrir, escribir, guardar y salir con las teclas) la hace el
    usuario a mano en la consola.
- **Bloqueos:** ninguno.

### 2026-10-18 — Fase 6: VERIFICADA ✅ (self-hosting en C: `tcc` se compila a sí mismo dentro de MiOS)

- **Hecho:**
  - **Las fuentes de `tcc` van dentro de la ISO**: `tools/build-tcc.sh` copia a `/usr/src/tcc/`
    los `.c`/`.h`/`.def` de la raíz mas `lib/` e `include/` (54 ficheros, ~1,4 MB), incluido el
    `config.h` que genera el propio script. Con eso el compilador puede compilarse a sí mismo sin
    salir de MiOS.
  - **`ONE_SOURCE`** (el valor por defecto de `tcc.h`) hace que baste **un fichero**: `tcc.c`
    incluye `libtcc.c`, y éste incluye el resto (preprocesador, generador, ELF, ensamblador,
    `x86_64-gen/link`…). La orden queda: `tcc -o /tmp/tcc2 /usr/src/tcc/tcc.c`.
  - **Corrección en `libtcc1.a`** (fue lo que falló en el primer intento): la `stdarg.h` de tcc
    para x86_64 manda `va_start`/`va_arg` a `__va_start`/`__va_arg`, que viven en `lib/va_list.c`.
    Sin ese objeto el enlazado se quedaba en
    `tcc: error: undefined symbol '__va_start'`. Ahora `libtcc1.a` lleva el juego completo de
    x86_64 del `lib/Makefile` de tcc (`libtcc1.o`, `va_list.o`, `alloca86_64.o`,
    `alloca86_64-bt.o`), sin `bcheck.o` (el modo de comprobación de límites está apagado por
    `CONFIG_TCCBOOT`). Pasó de 4 358 a **7 188 B**.
  - `initramfs/etc/autorun` vuelve a quedar limpio, con el ejemplo del self-hosting comentado (tarda
    un par de minutos en la VM, así que no conviene dejarlo activo en el arranque normal).

- **Verificación:**
  - ✅ **Build en el VPS OK** (9 pasos) → `MiOS.iso` **21 188 608 B**, md5
    `8da79f1a40d0e934fe6ef877c22ebe28` (local y remoto coinciden), ya en `Downloads`, con arranque
    limpio (banner MiOS 0.5 → prompt `MiOS>`).
  - ✅ **Self-hosting en C** (log serie de la VM real, con `/etc/autorun` =
    `runlog tcc -o /tmp/tcc2 /usr/src/tcc/tcc.c`, `runlog /tmp/tcc2 -o /tmp/hola2 /usr/src/hola.c`
    y `runlog /tmp/hola2`):
    `runlog: 'tcc' termino con codigo 0` (**tcc compila tcc**),
    `runlog: '/tmp/tcc2' termino con codigo 0` (**el tcc hecho por tcc compila un programa**) y
    `runlog: '/tmp/hola2' termino con codigo 0` con las seis comprobaciones `OK` y
    `[tcc] resultado: 0 fallo(s)` (**ese programa funciona**).
  - ✅ **Contenido real de la ISO**: `/usr/src/{hola.c,tcc/}` (54 ficheros, `config.h` presente,
    `lib/` incluido), `/lib/tcc/libtcc1.a` 7 188 B, `/bin/tcc` 271 312 B con intérprete
    `/lib/ld-mios.so.1` y `NEEDED libmios.so.6`.

- **Falta (fases siguientes):** binutils propias (`as`, `ld`) para GCC (Fase 7), GMP/MPFR/MPC
  (Fase 8), GCC etapa C (Fase 9), libstdc++ + GCC C++ (Fase 10) y el bootstrap nativo (Fase 11).

- **Bloqueos:** ninguno.

- **Notas:** el `tcc` de la ISO lo produce `mios-gcc` sin optimizar, así que compilar `tcc` dentro de
  MiOS tarda un par de minutos en la VM (es un compilador rápido, pero el binario no lo está). No
  hace falta una segunda generación: el hito es que el compilador de MiOS se reconstruye a sí mismo.

### 2026-10-18 — Fase 5: VERIFICADA ✅ (tcc: compilador de C dentro de MiOS)

- **Hecho:**
  - **`tcc` (Tiny C Compiler) 0.9.27, compilado con `mios-gcc` y metido en la ISO.**
    Las fuentes van en `third_party/tcc` (ignoradas por git; `tools/fetch-tcc.sh` las baja
    fijando la version) y `tools/build-tcc.sh` lo compila e instala. tcc se compila en
    **una sola unidad de traduccion** (tcc.c incluye libtcc.c, `ONE_SOURCE=1`) y se enlaza
    contra libmios como cualquier programa de MiOS.
  - **La adaptacion de tcc a MiOS es `third_party/tcc/config.h`** (lo genera
    `build-tcc.sh`): `CONFIG_TCCDIR=/lib/tcc`, `CONFIG_TCC_ELFINTERP=/lib/ld-mios.so.1`,
    `CONFIG_TCC_CRTPREFIX=/usr/lib`, `CONFIG_TCC_LIBPATHS`,
    `CONFIG_TCC_SYSINCLUDEPATHS`, `CONFIG_TCC_STATIC` (en MiOS no hay `dlopen`) y
    `CONFIG_TCCBOOT` (quita backtrace y comprobacion de limites, que necesitarian glibc).
  - **`build.sh` pasa a 9 pasos**: el 6 compila tcc e instala en el initramfs el
    compilador (`/bin/tcc`), sus cabeceras privadas (`/lib/tcc/include`), `libtcc1.a`,
    las cabeceras publicas de MiOS (`/usr/include`), el arranque (`/usr/lib/crt1.o` y
    `crti.o`/`crtn.o` vacios) y los enlaces `/lib/libc.so` y `/lib/libm.so` →
    `libmios.so.6`. Si no estan las fuentes de tcc, el paso se salta con un aviso (el
    resto de MiOS no depende de el).
  - **Libc ampliada para tcc**: `setjmp`/`longjmp` propios (`src/lib/setjmp.c`; guarda
    rbx/rbp/r12-r15 + rsp/rip, con jmp_buf propio de MiOS), `mmap`/`munmap`/`mprotect`
    como nombres POSIX sobre `mios_mmap`/... (`src/lib/posix.c`), `strtold` (delega en
    `strtod`) y cabeceras nuevas: `<setjmp.h>`, `<stdint.h>`, `<inttypes.h>`,
    `<sys/mman.h>`, `<sys/time.h>`.
  - **Cabeceras publicas portables (gcc *y* tcc)**: `stdio.h`/`mios.h` declaran las
    funciones de argumentos variables con `va_list` (de `<stdarg.h>`) en vez de
    `__builtin_va_list`, y `math.h` recurre a funciones de libmios (`__mios_isnan`,
    `__mios_huge_val`, `__mios_nan`, ...) cuando el compilador no es gcc. Ese era
    exactamente el primer error que daba tcc: `stdio.h:65: invalid type`.
  - **`/bin/runlog`** (nuevo, `userland/runlog.c`): ejecuta un programa y copia su salida
    (stdout **y** stderr) a la pantalla y al puerto serie. Es la herramienta con la que se
    depuro tcc sin pantalla:
    `runlog tcc -o /tmp/hola /usr/src/hola.c`.
  - **`examples/tcc-hola.c`** se instala como `/usr/src/hola.c`: programa de prueba
    (bucle y suma, cadenas, `sqrt`, `malloc`, `sizeof`, `argv`) que informa con `cons_*` y
    termina con codigo distinto de cero si algo falla.

- **Verificación:**
  - ✅ **Build en el VPS OK** (9 pasos) → `MiOS.iso` 20 844 544 B, md5
    `26eb5d7c0a4383e69e106ef7827a81c6` (local y remoto coinciden).
  - ✅ **tcc compila un `.c` dentro de MiOS y el binario resultante se ejecuta** (log serie
    de la VM real, con `/etc/autorun` = `runlog tcc -o /tmp/hola /usr/src/hola.c` y
    `runlog /tmp/hola`):
    `runlog: 'tcc' termino con codigo 0`, despues las seis comprobaciones `OK` y
    `[tcc] resultado: 0 fallo(s)` (bucle/suma, `strcpy`/`strcat`/`strcmp`, `sqrt`,
    `malloc`/`free`, `sizeof(void *)`, `argv`), y por ultimo
    `runlog: '/tmp/hola' termino con codigo 0`. El binario producido por tcc es un
    ejecutable dinamico: lo carga `ld-mios.so.1` y usa `libmios.so.6`.
  - ✅ **Contenido real de la ISO** (extrayendo el initramfs de dentro con `xorriso` +
    `cpio`): `/bin/{tcc,hola,args,selftest,runlog}`,
    `/lib/tcc/{include/{float,stdarg,stdbool,stddef,varargs}.h,libtcc1.a}`,
    `/usr/include/*` (+ `sys/`), `/usr/lib/{crt1.o,crti.o,crtn.o}`, `/usr/src/hola.c`,
    `/lib/{libmios.so.6,ld-mios.so.1,libc.so,libm.so}`.
  - ✅ **`readelf` de `/bin/tcc` (de la ISO)**: intérprete `/lib/ld-mios.so.1` y
    `NEEDED libmios.so.6`; dentro del binario se leen `/lib/ld-mios.so.1` (el intérprete
    que tcc escribe en lo que compila) y su ruta de cabeceras
    (`/lib/tcc/include:/usr/local/include:/usr/include`). `/bin/tcc` ocupa 271 312 B.
  - ✅ **Arranque final con `/etc/autorun` limpio**: banner MiOS 0.5 → prompt `MiOS>`, sin
    panic ni salida inesperada.

- **Falta (fases siguientes):** que **tcc se compile a si mismo** dentro de MiOS
  (Fase 6) y despues binutils, GMP/MPFR/MPC, GCC, libstdc++ y el bootstrap nativo.

- **Bloqueos:** ninguno.

- **Notas / desviaciones:**
  - tcc se compila **sin `make`** (el VPS no lo tiene y no hace falta): `build-tcc.sh`
    llama a `mios-gcc` directamente y genera el `config.h` que tcc espera de `configure`.
  - `runlog` no es un capricho de depuracion: en MiOS el stdout de un programa es la
    consola del sistema (tty0) y solo los mensajes `cons_*` se copian al puerto serie; con
    `runlog` quedan registrados en el log la salida *y los errores* de cualquier programa.
  - `strtold` delega en `strtod`: la precision de `long double` en esa ruta es la de
    `double` (limite conocido, anotado en el codigo).

### 2026-10-18 — Fase 4: VERIFICADA ✅ (procesos + ejecución de programas desde la shell)

- **Hecho:**
  - **La shell de MiOS ya lanza programas.** `src/core/shell.c`: si la orden no es un comando
    interno, se intenta ejecutar como programa (busca en `PATH`). Antes decía «Comando
    desconocido».
  - **`src/core/prog.c` (nuevo)**: `prog_run(cmd, args)` = `fork()` + `execvp()` en el hijo y
    `waitpid()` en el padre; devuelve el código de salida y, si es distinto de cero, lo dice
    (como un shell: silencio si todo va bien). Antes de bifurcar se vacían los buffers (consola
    y stdio) para que el hijo no los duplique.
  - **Comandos internos nuevos**: `cd [dir]` y `pwd` (`src/core/cmd_shell.c`).
  - **Entorno mínimo al arrancar** (`src/core/boot.c`): `PATH=/bin:/usr/bin`, `PWD=/`, `HOME=/`
    (con `setenv`, que ya funcionaba con `environ` a NULL).
  - **`include/unistd.h`**: se declaran `execv()` y `execvp()` (existían en `src/lib/process.c`
    pero no estaban en la cabecera pública).
  - **Programas de usuario** en `userland/`: `hola.c` (saludo + `uname` + `getpid`/`getppid`),
    `args.c` (imprime sus argumentos y `PATH`, y **termina con código = `argc`**) y `selftest.c`
    (**autoprueba** que comprueba `fork`/`waitpid`, `execv`/`execvp`, `pipe`, descriptores
    heredados y `getpid`/`getppid`; informa con `cons_*`, así que su salida se ve en pantalla *y*
    en el puerto serie). `build.sh` los compila a `initramfs/bin/`.
  - **`build.sh` reestructurado a 8 pasos** (antes 7): el `mk-sysroot.sh` pasa al paso 4 (antes
    del empaquetado) y se añaden el paso 5, que compila `userland/` a `initramfs/bin/`, y el 6,
    que instala la libc y el cargador. Así los programas existen ya al empaquetar el initramfs.
  - **`/etc/autorun` (nuevo)**: fichero opcional con órdenes que la shell ejecuta al arrancar
    (una por línea; `#` comenta). Se entrega sin órdenes (sólo comentarios) pero se **verificó con
    órdenes reales**. `src/core/shell.c`: `shell_autorun()`.
  - **`examples/fase4.c`** y **`tools/test-fase4.sh`**: prueba de `fork`/`wait`/`execv`/`execvp`/
    `pipe`/descriptores heredados en el `chroot` del sysroot.
- **Falta:**
  - Lo propio de la Fase 5 (portar `tcc`).
  - **Decisión abierta**: `initramfs/init` sigue versionado en git y es un artefacto que cambia en
    cada build. Mientras no se destrackee, se ha añadido su exclusión a `sync-vps.bat`/`sync-vps.sh`
    para que la copia vieja de Windows no pise la recién compilada en el VPS.
- **Verificación:**
  - ✅ **Build en el VPS OK** (`bash build.sh`, 8 pasos, sin *warnings* ni *errors*).
    `MiOS.iso` 20 684 800 B, md5 `843cbd394fcfd76e6da9b62040a3ab15` (local y remoto coinciden).
  - ✅ **`examples/fase4.c` en el `chroot` del sysroot: `resultado: 0 fallo(s)`** (código 0).
    Cubre: `fork`+`waitpid` y código de salida; `execv` de `/hola` (ruta absoluta); `execvp` con
    `PATH` y **argumentos** (`args uno dos` → sale con 3); `pipe` de hijo a padre con datos;
    `exec` de algo inexistente → 127; y que el hijo escribe por el mismo `stdout` heredado.
  - ✅ **Autotest DENTRO de MiOS, arrancando la VM de verdad** (log serie, `md5 843cbd…`): con
    `/etc/autorun` = `selftest`, el log serie registra las nueve comprobaciones **`OK`** y
    **`resultado: 0 fallo(s)`**:
    `fork + waitpid` · código del hijo (7) · `execv /bin/hola` → 0 · `execvp args` (PATH) → 3 ·
    `execv` inexistente → 127 · tubería hijo→padre · el hijo hereda la consola ·
    `getpid()`/`getppid()`. Es decir: **la shell de MiOS lanzó `/bin/selftest`, y éste lanzó a su
    vez `/bin/hola` y `/bin/args`** y recogió sus estados.
  - ✅ **Lanzamiento de verdad DENTRO de MiOS** (log serie): con `/etc/autorun` conteniendo
    `pwd`, `hola` y `args uno dos`, el arranque registró `/` (`pwd`), `MiOS: 'args' termino con
    codigo 3` y la salida de `ls` tras `cd /bin` (`hola`, `args`) → la shell de MiOS lanzó
    `/bin/args` con sus argumentos y **recogió su código de salida**.
  - ✅ **`bin/` y `etc/` en la ISO**: extrayendo el initramfs **de la propia ISO** (`xorriso` +
    `cpio`) aparecen `bin/hola`, `bin/args`, `bin/selftest` y `etc/autorun`.
  - ✅ **Arranque final con `/etc/autorun` limpio**: banner MiOS 0.5 → prompt `MiOS>`, sin panic ni
    salida inesperada.
  - ⏳ Consola interactiva dentro del SO: la prueba a mano la hace el usuario (`run-vm.bat`);
    p. ej. teclear `hola` o `args uno dos` en el prompt.
- **Nota de comportamiento (importante):** la salida de un **programa** va a su `stdout`
  (fd 1 = `/dev/console` = **tty0, la pantalla VGA**). Los mensajes de MiOS, además, se
  **duplican al puerto serie** porque `src/lib/console.c` los escribe también en `/dev/ttyS0`.
  Por eso en el log serie se ven los mensajes del shell (`MiOS: 'args' termino con codigo 3`) pero
  no el `printf` de `/bin/hola`, que sí aparece en la pantalla.
- **Bloqueos:** ninguno.

### 2026-10-18 — Fase 3: VERIFICADA ✅ (`libm` propia + `fenv`)

- **Hecho:**
  - **`src/lib/math.c` (nuevo)**: biblioteca matemática propia dentro de `libmios.so.6` (no hay
    `libm` aparte). Estrategia mixta:
    - **Exacto** (bits o unidad x87): `fabs`/`copysign`/`signbit`, `frexp`/`ldexp`/`scalbn`/
      `scalbln`, `modf`, `ilogb`/`logb`, `nextafter`/`nexttoward`, `sqrt` (`sqrtsd`/`sqrtss`/
      `fsqrt`), y `floor`/`ceil`/`trunc`/`rint`/`nearbyint` con **`frndint`** + la palabra de
      control x87 (`rint`/`nearbyint` respetan el modo actual).
    - **Trascendentes** con reducción de argumento + polinomios minimax tipo fdlibm
      (`sin`/`cos`/`atan`/`asin`/`acos`, `atan2`) y series reducidas (`exp`/`expm1`/`exp2`,
      `log`/`log1p`/`log2`/`log10`, `pow` con camino exacto para exponente entero,
      `sinh`/`cosh`/`tanh` y sus inversas, `cbrt` por Newton, `hypot`, `fma`).
    - **Gamma** (`lgamma_r`/`lgamma`/`tgamma`) por **Lanczos** (g=7) con reflexión, y
      **`erf`/`erfc`** por serie (|x|<3) + **fracción continua de Lentz** (|x|≥3; la serie
      asintótica no daba precisión suficiente en `erfc`).
    - Variantes `f` (delegan en `double`, redondeo final) y `l` (x87 donde es exacto: `sqrtl`,
      `floorl`, `fmodl`, `frexpl`…; `double` en las trascendentes — **precisión suficiente, no
      perfecta**, documentado).
  - **`src/lib/fenv.c` (nuevo)**: entorno de coma flotante sobre **x87 + MXCSR** (sincronizados:
    vale igual para `double` con SSE y `long double` con x87). `fegetround`/`fesetround` (afecta
    de verdad a `rint`/`nearbyint`), `fegetenv`/`fesetenv`/`feholdexcept`/`feupdateenv`,
    `feclearexcept`/`fetestexcept`/`feraiseexcept`/`fegetexceptflag`/`fesetexceptflag`. `fnstenv`
    enmascara excepciones al guardar, así que tras usarlo se recarga el control con `fldcw`.
  - **Cabeceras públicas nuevas**: `include/math.h` (constantes `M_*`, `HUGE_VAL*`, `INFINITY`,
    `NAN`, `FP_*`, macros `isnan`/`isinf`/`isfinite`/`isnormal`/`signbit`/`isgreater`… vía
    `__builtin_*`, ~40 funciones en las tres variantes, `FP_ILOGB*`) e `include/fenv.h`
    (`fenv_t` propia `{cw, sw, mxcsr}` y `FE_*`).
  - **`mk-sysroot.sh`**: crea `usr/lib/libm.so` y `usr/lib/libc.so` como enlaces a
    `libmios.so.6`, de modo que `-lm` (y `-lc`) enlazan sin `libm` aparte.
  - **`examples/fase3.c`** (~60 comprobaciones) y **`tools/test-fase3.sh`** (lo compila **con
    `-lm`** y lo ejecuta en el `chroot` del sysroot).
- **Falta:**
  - Lo propio de la Fase 4 (procesos/FS: lanzar programas desde la shell de MiOS).
  - Decidir el destino del `initramfs/init` versionado (sigue en git; `build.sh` lo sobrescribe).
- **Verificación:**
  - ✅ **Build en el VPS OK** (`bash build.sh`, 7 pasos, sin *warnings* y sin *errors*;
    `MiOS.iso` 20 680 704 B, md5 `bca4699db0aa6865ce8c51af9417b040`; local y remoto coinciden).
  - ✅ **`examples/fase3.c` ejecutado en el `chroot` del sysroot: `resultado: 0 fallo(s)`**
    (código de salida 0, enlazado con `-lm`; ~85 comprobaciones). Cubre: raíces y `cbrt`/`hypot`,
    redondeo (`floor`/`ceil`/`trunc`/`round` ties-away vs `rint` ties-even), resto (`fmod`/`remainder`,
    incl. negativos y `fmodl`), descomposición (`frexp`/`ldexp`/`modf`/`ilogb`), `exp`/`log`/
    `exp2`/`log2`/`log10`/`log1p`/`expm1`, `pow` (entero, fraccionario, base negativa, negativo),
    trigonometría (`sin`/`cos`/`tan`/`atan`/`atan2`/`asin`/`acos` y variantes `f`/`l`),
    hiperbólicas y sus inversas, `tgamma`/`lgamma`/`erf`/`erfc`, clasificación
    (`isnan`/`isinf`/`isfinite`/`fpclassify`/`fmax`/`fmin`/`fdim`/`fma`/`nextafter`/`nan`), y
    **valores difíciles** en rango amplio (`exp(±700)`, `exp2(-100)`, `log(1e300)`, `log2(0.001)`,
    `pow(2,-1074)`, `sin/cos/tan(100)`, `hypot(1e±200)`, `cbrt(1e-30)`, `asinh(1e10)`,
    `tgamma(-0.5)`, `lgamma(100)`, `erfc(5)`, `erfc(6)`, negativos) con tolerancia relativa; y
    **`fenv`** (modo por defecto, `FE_DOWNWARD`/`FE_UPWARD`/`FE_TOWARDZERO` afectando a `rint`,
    `feclearexcept`/`fetestexcept`, `fegetenv`/`feholdexcept`/`feupdateenv`).
  - ✅ **`-lm` comprobado**: `./mios-gcc examples/fase3.c -o build/fase3 -lm` enlaza y el `NEEDED`
    resultante es `libmios.so.6` (el enlace `libm.so` del sysroot lleva al SONAME correcto).
  - ✅ **Arranque de MiOS en VMware**, leyendo el log serie: `Run /init as init process` → banner
    MiOS 0.5 → prompt `MiOS>` → 7 módulos de red `cargado`, **sin panic**.
  - ⏳ Consola interactiva dentro del SO: la prueba a mano la hace el usuario (`run-vm.bat`).

### 2026-10-18 — Fase 2: VERIFICADA ✅ (`libmios` es ya una libc C/POSIX usable)

- **Hecho:**
  - **`errno` en todas las syscalls**: `src/kernel/syscall.c` pasa por un ayudante `sys_ret()` que
    deja el código en `errno` (el almacenamiento vive en `src/lib/errno.c` con
    `__errno_location()`; se quitó de `posix.c` para que sea único). `mios_strerror` ampliado.
  - **Syscalls nuevas** (`src/kernel/kernel.h` + `syscall.[ch]`): `mmap` `munmap` `mprotect` `brk`
    `lseek` `dup` `dup2` `pipe` `fcntl` `ioctl` `access` `newfstatat` (`stat`/`lstat`) `fstat`
    `fchmod` `chmod` `fsync` `link` `symlink` `readlink` `chown` `umask` `truncate` `ftruncate`
    `time` `gettimeofday` `clock_gettime` `clock_getres` `rt_sigaction` `rt_sigprocmask` `fork`
    `execve` `wait4` `kill` `setsid` `setpgid` `getpgid` `getppid` `alarm` `socket` `bind`
    `sendto` `recvfrom` `getrandom`. `struct stat`/`timeval`/`dirent64` de x86_64 en `kernel.h`
    (con guardas compartidas con la cabecera pública para no redefinirlas).
  - **`src/lib/malloc.c` (nuevo)**: `malloc/calloc/realloc/free` propios sobre `brk`, con cabecera
    de 32 bytes (payload alineado a 16), *first fit*, división del sobrante y **fusión de
    vecinos**. Al ser el `brk` contiguo, los bloques siempre son adyacentes y la fusión es segura.
  - **`src/lib/format.c` (nuevo)**: motor de formato único con un **emisor abstracto**
    (`mios_vformat(put, ctx, fmt, ap)`): la consola le pasa `cons_putc` y `snprintf` un buffer.
    Soporta `%d %i %u %x %X %o %c %s %p %n %f %e %g` (y mayúsculas) con banderas `- + espacio 0 #`,
    ancho, precisión y modificadores `hh h l ll z j t L`. `cons_vprintf()` ahora es solo un
    emisor sobre la consola (desapareció el contador duplicado `console_count`).
  - **`src/lib/stdio.c` (nuevo)**: `FILE` real con buffer propio (tabla estática, **sin malloc**),
    `stdin`/`stdout`/`stderr` como **símbolos de datos**; `fopen fdopen fclose fflush fread fwrite
    fgetc getc getchar ungetc fputc putc putchar fgets fputs puts feof ferror clearerr fileno
    fseek ftell rewind setvbuf setbuf printf fprintf vfprintf vprintf sprintf snprintf vsprintf
    vsnprintf scanf fscanf sscanf tmpfile remove perror`.
  - **`src/lib/scanf.c` (nuevo)**: `vsscanf`/`vfscanf` con una fuente común (cadena o `FILE`) y
    `%d %i %u %o %x %X %p %c %s %e %f %g %n %%` con ancho, `*` y modificadores.
  - **`src/lib/ctype.c`, `stringstd.c`, `stdlib.c`, `dirent.c`, `stat.c`, `time.c`, `process.c`,
    `misc.c` (nuevos)**: `ctype.h` completa; `strcat strncat stpcpy strrchr strstr strtok(_r)
    strdup strndup strspn strcspn strpbrk strcasecmp strncasecmp strnlen memchr`;
    `strtol/strtoll/strtoul/strtoull/strtod/strtof/atof/atoi/atol/atoll` (con `endp` de verdad),
    `qsort` (quicksort + inserción), `bsearch`, `getenv/setenv/putenv/unsetenv` sobre `environ`,
    `atexit`/`__cxa_atexit`/`__cxa_finalize`, `exit` (ejecuta los `atexit`, vacía stdio y consola),
    `rand/srand`, `abs/labs/llabs/div/ldiv/lldiv`; `opendir/readdir/closedir/rewinddir/telldir/
    seekdir` sobre `getdents64`; `stat/fstat/lstat/chmod/fchmod/umask/access/dup/dup2/pipe/lseek/
    ioctl/fcntl/isatty/pread/pwrite/truncate/ftruncate/fsync/link/symlink/readlink/chown`;
    `time/gettimeofday/clock_gettime/clock_getres/clock/nanosleep/difftime/gmtime(_r)/localtime(_r)/
    mktime/asctime/ctime/strftime` (calendario con `days_from_civil`/`civil_from_days`; **sin husos
    horarios**, todo UTC); `fork/execv/execvp/execve/wait/waitpid/getppid/setsid/setpgid/getpgid/
    kill/usleep` y `signal/raise` (con el **restaurador `SA_RESTORER`** que exige x86_64);
    `__assert_fail`, `__stack_chk_fail`, `__stack_chk_guard`.
  - **Cabeceras públicas nuevas**: `ctype.h`, `dirent.h`, `sys/types.h`, `sys/stat.h`, `sys/wait.h`,
    `time.h`, `assert.h`, `limits.h`, `signal.h`; y ampliadas `stdio.h`, `stdlib.h`, `string.h`,
    `unistd.h`, `fcntl.h`, `errno.h`, `mios.h` (todas con guardas de typedef para poder incluirlas
    en cualquier orden).
  - **`crt1.c`**: guarda `argc/argv/envp` en `rbx/r12/r13` (sobreviven a las llamadas), llama a
    `__mios_set_environ(envp)` — así `getenv`/`environ` funcionan — y luego a `main()`.
  - **¡El cargador implementa `R_X86_64_COPY`!** (`src/ld/ldmios.c`): los datos que el ejecutable se
    queda copiados (`stdout`, `environ`…) se rellenan desde la librería que los define. Además se
    arregló la **interposición**: un símbolo global definido *dentro* de la librería pero copiado
    por el ejecutable debe resolverse **a la copia del ejecutable** (si no, `libmios` escribiría en
    su propia variable y el programa leería otra). Los símbolos locales y `RELATIVE` siguen siendo
    del módulo; si la resolución global falla, se usa la definición propia.
  - **`mk-sysroot.sh`**: copia las cabeceras con subdirectorios (`cp -a include/.`), ahora que hay
    `sys/stat.h` y `sys/types.h`.
  - **`examples/fase2.c`** (test grande de la libc) y **`tools/test-fase2.sh`** (lo compila y lo
    ejecuta en el `chroot` del sysroot en el VPS).
- **Falta:**
  - Lo propio de la Fase 3 (`libm`: `math.h` y compañía).
  - Decidir el destino del `initramfs/init` versionado (sigue en git; `build.sh` lo sobrescribe).
- **Verificación:**
  - ✅ **Build en el VPS OK** (`bash build.sh`, 7 pasos, sin *warnings*, `MiOS.iso` 20 664 320 B,
    md5 `a404bb4ba8375d1e8e82905d35320f42`; local y remoto coinciden).
  - ✅ **`examples/fase2.c` ejecutado en el `chroot` del sysroot: `resultado: 0 fallo(s)`** (código
    de salida 0). Cubre: 6 pruebas de memoria dinámica (incluye 200 `malloc` encadenados,
    `realloc` que conserva datos, `calloc` a cero), `snprintf` con todos los tipos
    (`-7/hola/3.14/00ff/Z`) y `sscanf` de 4 campos, cadenas/`ctype`, `qsort`+`bsearch`, entorno,
    `fopen/fprintf/fputs/fwrite/ftell/fgets/fseek/fread/feof/remove/fclose` con `errno=ENOENT`,
    `stat`/`opendir`/`readdir` (`/lib` con 5 entradas y `libmios.so.6` encontrado), `time`/`gmtime`/
    `strftime` (`2026-10-18 06:34:45`), `uname`, `lseek`, `isatty`, y `atexit` al salir.
  - ✅ **`R_X86_64_COPY` comprobado con `readelf -r`**: `stdout` y `environ` aparecen como COPY en el
    ejecutable, y *ambos funcionan* (`fprintf(stdout, ...)` y `environ == envp`).
  - ✅ **Arranque de MiOS en VMware**, leyendo el log serie: `Run /init as init process` → banner
    MiOS 0.5 → prompt `MiOS>` → 7 módulos de red `cargado`, **sin panic** (el cargador cambió:
    era lo más arriesgado de esta fase).
  - ⏳ Consola interactiva dentro del SO: la prueba a mano la hace el usuario (`run-vm.bat`).

### 2026-10-18 — Fase 1: VERIFICADA ✅ (cualquier programa se compila con `mios-gcc` y corre contra `libmios`)

- **Hecho:**
  - **Cabeceras publicas** en `include/` (nuevo): `mios.h` (paraguas), `mios_types.h`,
    `stdio.h`, `stdlib.h`, `string.h`, `unistd.h`, `fcntl.h`, `errno.h`. Usan los nombres
    estandar y van a `<sysroot>/usr/include`. No incluyen ninguna cabecera del sistema: solo
    `<stddef.h>`, que lo aporta el propio gcc en modo *freestanding*.
  - **`src/lib/posix.c` (nuevo, dentro de `libmios.so.6`)**: los nombres estandar que faltaban
    (`printf`, `puts`, `putchar`, `open`, `read`, `write`, `close`, `exit`, `_exit`, `abort`,
    `mkdir`, `rmdir`, `rename`, `unlink`, `uname`, `getpid`, `getcwd`, `sleep`, `strlen`,
    `strcmp`, `strcpy`, `strchr`, `atoi`, `strtoul`, `strerror`, `abs`...). Traducen el
    convenio del kernel (`-errno`) al de C (`-1` + `errno`).
  - **`errno` se resuelve con `__errno_location()`**, no con una variable global: el cargador
    propio **todavia no implementa `R_X86_64_COPY`**, asi que un simbolo de datos importado de
    la `.so` acabaria leyendo una copia sin actualizar dentro del ejecutable.
  - **`cons_vprintf()` expuesto** en `src/lib/console.[ch]`: el motor de formato se separo de
    `cons_printf` (que ahora es un envoltorio) para que `printf()` pueda reenviar sus varargs.
    Ademas el motor ya **cuenta los caracteres** escritos.
  - **`src/crt/crt1.c` (nuevo)**: el `_start` de los programas normales. Coge `argc/argv/envp`
    de la pila que deja el cargador, llama a `main()` y termina con `exit()` (que vacia la
    consola). `init` conserva su propio `_start` (`src/core/main.c`), porque nunca vuelve.
  - **`mios-gcc` (nuevo)**: envoltorio sobre el `gcc` del VPS. Anade el sysroot, los flags
    `-ffreestanding -fno-builtin -fno-pie`, y al enlazar `crt1.o` + `-lmios` +
    `-Wl,--dynamic-linker=/lib/ld-mios.so.1`. Detecta `-c/-S/-E/-M` y en esos casos no enlaza.
    `libgcc` se pasa por ruta absoluta (`--sysroot` tambien afecta a las rutas de busqueda).
  - **`mk-sysroot.sh` (nuevo)**: monta `sysroot/` = cabeceras + `crt1.o` + `libmios.so.6` +
    `ld-mios.so.1`, en `usr/lib` y **tambien en `lib/`** (las rutas que usa el cargador en
    ejecucion). El sysroot sirve como **raiz ejecutable**:
    `cp prog sysroot/ && sudo chroot sysroot /prog` (sin montajes, asi que es seguro).
  - **`build.sh`**: ahora tiene **7 pasos**; el 7 monta el sysroot, compila
    `examples/hola.c` con `mios-gcc` y muestra su `INTERP`/`NEEDED` como comprobacion.
  - **`examples/hola.c`** (programa de ejemplo) y **`examples/fmt.c`** (prueba del motor de
    formato que usa el shell: `%-10s`, `%02d`, `%lu`, `%x`, `%c`, negativos...).
  - `sync-vps.sh`/`sync-vps.bat` excluyen `sysroot/` y `build/`; `.gitignore` ignora `sysroot/`.
- **Falta:**
  - Lo propio de la Fase 2: completar la libc (memoria dinamica `malloc/free`, `errno` en las
    syscalls, mas de `string.h`, ficheros con `FILE`...).
  - Decidir el destino del `initramfs/init` versionado (sigue en git; `build.sh` lo sobrescribe).
- **Verificación:**
  - ✅ **Build en el VPS OK** (`bash build.sh`, 7 pasos, `MiOS.iso` 20 633 600 bytes,
    md5 `b81ae2f5c9191c40dcf9137fbea955ec`; el md5 local y el remoto coinciden).
  - ✅ **`mios-gcc examples/hola.c -o hola`** produce un **ELF dinamico**:
    `interpreter /lib/ld-mios.so.1`, `NEEDED libmios.so.6`.
  - ✅ **Ejecutado de verdad** en el VPS, en el chroot del sysroot (sin montajes):
    ```
    === hola desde MiOS ===
    argc = 3
      argv[0] = "/hola"
      argv[1] = "uno"
      argv[2] = "dos"
    Hola, libmios!
    uname: Linux 6.8.0-100-generic x86_64
    strlen("abc") = 3
    2 + 3 = 5
    ```
    (argc/argv desde el kernel, `printf`, `uname` y `exit` funcionando.)
  - ✅ **`fmt.c`**: todos los formatos correctos (`[abc] [       abc] [abc       ]`,
    `[06] [   42] [42   ] [00000042]`, `-1234567890123456789`, `beef`, `%`).
  - ✅ **Arranque de MiOS en VMware**, verificado leyendo el log serie: `Run /init as init
    process` → banner MiOS 0.5 → prompt `MiOS>` → 7 modulos de red `cargado`, **sin panic**
    (el refactor de `console.c` no rompio `init`).
  - ✅ 37 simbolos estandar nuevos exportados por `libmios.so.6` (comprobado con `nm -D`).
- **Bloqueos:** ninguno.

### 2026-10-18 — Fase 0: VERIFICADA ✅ (arranca en VMware, sin kernel panic)

- **Hecho:**
  - **Corregido un bug real del cargador** que provocaba *kernel panic* nada más arrancar:
    `traps: init[1] general protection fault ... in libmios.so.6` →
    `Kernel panic - not syncing: Attempted to kill init! exitcode=0x0000000b`.
  - **Causa raíz.** El segmento RW de `libmios.so.6` termina a mitad de página
    (`LOAD 0x3ee8 vaddr 0x4ee8 filesz 0x1d0 memsz 0x10318`): `.data` acaba en `vaddr 0x50b8`
    y `.bss` empieza en `0x50c0` **en la misma página**. El kernel resuelve esto con
    `padzero(elf_bss)`: mapea la página desde el fichero y **pone a cero el sobrante**. El
    cargador mapeaba el fichero hasta el final de la página **sin poner a cero ese trozo**, así
    que `.bss` (`log_pos` y el `log_buffer` de 64 KB) arrancaba con lo que sigue en el fichero:
    literalmente la sección `.comment` = `"GCC: (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0"`, que es
    exactamente la "basura" que salía en los registros (`0x756275362d302e33` = `"3.0-6ubu"`).
  - **Arreglo** en `src/ld/ldmios.c` (`load_library`): se replican las cuentas del kernel
    (`dend = inpage + p_filesz`, `fend = align_up(dend)`, `mend = align_up(inpage + p_memsz)`).
    Si `dend < fend` se mapea el fichero con `prot | PROT_WRITE`, se pone a cero `[dend, fend)`
    (nuevo `memzero()`) y, si el segmento era de solo lectura, se restaura con `mprotect`
    (nueva syscall `SYS_mprotect`). El resto del `.bss` (`[fend, mend)`) sigue siendo anónimo a cero.
  - `iso/boot/grub/grub.cfg`: el kernel ahora arranca con `console=ttyS0,115200 console=tty0`.
    Se deja `tty0` **el último** a propósito: así `/dev/console` sigue siendo la pantalla (el
    usuario ve MiOS en la VM) y, además, los mensajes del kernel y los panics se copian al log
    serie `C:\Users\erne\mioos.log` (clave para depurar sin ver la pantalla).
  - Nuevos scripts para el usuario: `build-vps.bat` (sync + build en el VPS + trae la ISO) y
    `run-vm.bat` (arranca la VM de MiOS en VMware). Ya existían `sync-vps.bat` y `get-iso.bat`.
- **Falta:**
  - Decidir el destino del `initramfs/init` versionado (sigue en git; `build.sh` lo sobrescribe).
  - Empezar la Fase 1 (sysroot + `mios-gcc`).
- **Verificación:**
  - ✅ **Build en el VPS OK** (`bash build.sh`, 6 pasos, `MiOS.iso` 20 MB, md5 `22d68644...`).
  - ✅ **Arranque real en VMware Workstation (VM "Ubuntu", CD = `MiOS.iso`), verificado yo mismo**
    leyendo el log serie de la VM:
    - `[3.327482] Run /init as init process` → aparece el banner completo de MiOS 0.5 y el prompt
      `MiOS>`; se cargan los 7 módulos de red (`mii`, `pcnet32`, `e1000`, `e1000e`, `vmxnet3`,
      `realtek`, `r8169` → todos `cargado`).
    - Sin `panic`, sin `general protection fault`, sin `Attempted to kill init`.
  - ✅ El mismo `init` dinámico + `libmios.so.6` + `ld-mios.so.1` también se ejecutaron en un
    `chroot` del VPS (banner + shell), confirmando que el enlazado dinámico es correcto.
- **Bloqueos:** ninguno.

### 2026-10-18 — Fase 0: primera implementación (SIN VERIFICAR)

- **Hecho:**
  - Nuevo cargador propio `src/ld/ldmios.c` (`ld-mios.so.1`): lee el `auxv`, carga las `DT_NEEDED`
    de `/lib`, aplica relocalizaciones (`RELATIVE`, `GLOB_DAT`, `JUMP_SLOT`, `64`, `PC32`),
    ejecuta `DT_INIT_ARRAY` y salta a `AT_ENTRY`.
  - `build.sh` reescrito en 6 pasos: (1) `libmios.so.6` PIC desde `src/lib` + `src/kernel`,
    (2) `ld-mios.so.1`, (3) `init` dinámico enlazado con `-lmios`, (4) copia de lib + cargador al
    `initramfs`, (5) empaquetado cpio, (6) verificación + ISO.
  - `init` pasa a ser **ELF dinámico** (`PT_INTERP=/lib/ld-mios.so.1`, `DT_NEEDED=libmios.so.6`),
    **ET_EXEC** (base 0) para simplificar el cargador. El cargador se coloca en `0x40000000`
    (`-Wl,-Ttext-segment`) para no solaparse con `init` en `0x400000`.
  - `.gitignore`: se ignoran `build/`, `initramfs/lib/libmios.so.6` y `initramfs/lib/ld-mios.so.1`.
  - Documentado que `initramfs/dev/console` debe crearse como **dispositivo de caracteres**
    (`sudo mknod initramfs/dev/console c 5 1`), no con `touch`.
- **Falta:**
  - **Prueba funcional en QEMU** (la hace el usuario): arrancar `MiOS.iso` y usar la consola.
  - Decidir el destino del `initramfs/init` versionado (sigue en git).
- **Verificación:**
  - ✅ **Build en el VPS OK** (2026-10-18, `~/OS Nuevo test`): `./build.sh` completa los 6 pasos y genera
    `MiOS.iso` (20 MB). Evidencia (via `readelf`/`nm`):
    - `init`: `ELF 64-bit LSB executable, dynamically linked, interpreter /lib/ld-mios.so.1`;
      `NEEDED libmios.so.6`; 43 `R_X86_64_JUMP_SLOT` sin resolver (se resuelven desde la lib).
    - `libmios.so.6`: shared object; 64 simbolos exportados (`mios_*`, `cons_*`, `memcpy`, `memset`,
      `__udivdi3`); `DT_HASH` presente; 22 `JUMP_SLOT` internos.
    - `ld-mios.so.1`: `EXEC` estatico en `0x40000000`; **sin** seccion dinamica ni relocalizaciones.
    - initramfs: incluye `dev/console` (char 5,1), `lib/libmios.so.6`, `lib/ld-mios.so.1`, `init`
      y los 7 `.ko`.
    - Relocalizaciones presentes: **solo `R_X86_64_JUMP_SLOT`** (tipo 7), que el cargador implementa.
  - ⏳ **Prueba en QEMU**: pendiente (la hace el usuario).
- **Riesgos abiertos:**
  - Bajo: el kernel podria rechazar un interprete `ET_EXEC` (descartado por el propio `file`/`readelf`,
    pero se confirma al arrancar). Alternativa si falla: cargador static-PIE con autorrelocalizacion.
  - Bajo: comportamiento en runtime del cargador (aun no ejecutado en QEMU).
- **Bloqueos:** ninguno.

---

### 2026-10-18 — Incidencia: el `chroot` de prueba rompió el `/dev` del VPS (resuelta)

- **Qué paso:** para probar el cargador sin VM se ejecuto `sudo chroot /tmp/mt /init` en el VPS.
  MiOS monta `proc`/`sysfs`/`devtmpfs`, y **`devtmpfs` es un *singleton*** (todos sus montajes
  comparten superbloque), asi que al limpiar despues con `rm -rf /tmp/mt` **como root** con los
  montajes aun activos se borraron nodos del **`/dev` real**: `/dev/null` quedo como fichero normal
  (`-rw-r--r--`) y desaparecieron `/dev/zero`, `/dev/random`, `/dev/urandom`, `/dev/console`,
  `/dev/tty`, `/dev/ptmx`. Consecuencia: el `sshd` del VPS dejaba de aceptar conexiones
  (`kex_exchange_identification: read: Connection reset`, 0 bytes) porque no hay `/dev/null` valido.
- **Arreglo:** **reinicio del VPS** (lo hizo el usuario) → `devtmpfs` recrea `/dev` limpio.
  Verificado: `/dev/null` = `crw-rw-rw- 666 root:root 1:3`, sin montajes colgando en `/tmp/mt`, y la
  `MiOS.iso` intacta (md5 `22d68644...`).
- **Prevencion:** documentado en `AGENTS.md` («Ciclo de verificacion»): el atajo por `chroot` hay que
  hacerlo en un **namespace de montaje propio** (`unshare -m`) y **desmontar antes de borrar**; se
  incluye la receta y los `mknod` de rescate.
- **Leccion:** las pruebas con `chroot`+`mount` como root pueden afectar al host; preferir la VM
  (que es el entorno real de MiOS) y, si se usa `chroot`, con `unshare -m`.

---

<!--
Plantilla para nuevas entradas:

### AAAA-MM-DD — Fase N: <título>

- **Hecho:**
- **Falta:**
- **Verificación:** (build en el VPS / prueba del usuario en QEMU)
- **Bloqueos:**
-->
