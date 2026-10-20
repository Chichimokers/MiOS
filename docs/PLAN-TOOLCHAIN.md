# Plan — Toolchain propio de MiOS (tcc → GCC)

> Plan completo, de punta a cabo, para que MiOS tenga su **propia libc (`libmios`) dinámica**,
> su **propio cargador (`ld-mios.so.1`)** y su **propio toolchain**: primero `tcc` (C), después
> `GCC` con `libstdc++` (C++), hasta llegar al **self-hosting**.
>
> Bitácora de avances (qué se hizo y por dónde vamos): [`PROGRESO.md`](PROGRESO.md).

---

## 0. Cómo trabajamos (reglas fijas)

| Regla | Detalle |
|---|---|
| **Máquina de build** | Linux **`192.168.1.87`** (usuario `chichimokers`, contraseña `123`). En Windows se edita; en Linux se compila y se prueba. |
| **Sincronizar tras cada cambio** | `./sync-vps.sh` copia el repo al VPS para que **los dos coincidan**. |
| **Directorio en el VPS** | `/home/chichimokers/OS Nuevo test` (tiene espacios; se cita entre comillas). Alias: `~/OS Nuevo test`. |
| **Pruebas dentro del SO** | Las hace **el usuario a mano** (arrancar la ISO en QEMU/VMware y usar la consola de MiOS). El agente **no** las ejecuta: las pide explícitamente. |
| **Bitácora** | Al cerrar cada fase, añadir una entrada en [`PROGRESO.md`](PROGRESO.md). |

> Nota: la contraseña está en claro solo porque así lo pidió el usuario; para automatizar el
> `scp`/`rsync` sin teclear la clave, lo ideal es configurar una **clave SSH** una sola vez.

---

## 1. Resumen ejecutivo

| Punto | Decisión |
|---|---|
| Sistema | MiOS = userspace PID 1 sobre kernel Linux x86_64 |
| Meta final | `tcc` + `GCC` (C y C++) corriendo **dentro** de MiOS |
| Libc | propia: `libmios.so.6` (nada de glibc dentro de MiOS) |
| Cargador | propio: `ld-mios.so.1` |
| Compilador 1º | `tcc` (C, pequeño, as/ld propios, sin GNU) |
| Compilador 2º | `GCC` + `libstdc++` (C++, potencia; acepta libstdc++ de GNU) |
| GCC del VPS | **solo herramienta de build**, nunca entra en la ISO |
| Truco clave | MiOS corre sobre Linux ⇒ los binarios MiOS se **construyen y prueban en el VPS** |

**Frase guía:** *GCC es el albañil, no el inquilino.* Se usa para construir, no se instala.

---

## 2. Arquitectura objetivo

### Dentro de MiOS (la ISO / initramfs)

```
/lib/libmios.so.6          ← TU libc (C/POSIX-lite + syscalls)
/lib/ld-mios.so.1          ← TU cargador dinámico
/lib/libmios_net.so        ← (después) red
/lib/libmios_fs.so         ← (después) sistema de ficheros
/usr/include/...           ← TUS cabeceras
/bin/init                  ← PID 1, dinámico
/bin/tcc                   ← (Fase 5 ✅) compilador C propio
/lib/tcc/{include,libtcc1.a}  ← (Fase 5 ✅) cabeceras privadas y soporte de tcc
/usr/lib/{crt1.o,crti.o,crtn.o} ← (Fase 5 ✅) arranque de los programas que compila tcc
/usr/src/tcc/...           ← (Fase 6 ✅) fuentes de tcc: tcc se compila a sí mismo dentro de MiOS
/bin/runlog                ← (Fase 5 ✅) ejecuta un programa y copia su salida al puerto serie
/bin/as  /bin/ld           ← (Fase 7 ✅) binutils propias 2.42   [para GCC]
/lib/libgmp.so /lib/libmpfr.so /lib/libmpc.so           ← (Fase 8)
/bin/gcc  /lib/libstdc++.so                             ← (Fase 9-10)
```

### En el VPS (herramientas de build, NO entran en la ISO)

```
gcc / g++  (Ubuntu)        ← albañil
binutils, make, autoconf   ← albañil
grub, xorriso, mtools      ← genera la ISO
qemu-system-x86            ← pruebas
git, rsync/scp             ← sincronización
```

### Sysroot (el puente)

```
/mios/sysroot/
├── usr/include/           ← cabeceras de MiOS
├── usr/lib/
│   ├── crt1.o crti.o crtn.o   ← TU crt (arranque de programas)
│   ├── libmios.so → libmios.so.6
│   └── ld-mios.so.1
└── lib/                   ← enlaces (los binarios buscan /lib/...)
```

---

## 3. Estado de partida (lo que YA tienes)

| Elemento | Estado |
|---|---|
| `src/lib/` (console, string, memory, convert, errno) | ✅ existe, estático |
| `src/kernel/syscall.c` (~25 wrappers) | ✅ único sitio con `syscall` |
| `build.sh` (build estático, `-static -nostdlib -no-pie -ffreestanding`) | ✅ existe |
| `init` (48 KB, sin `.so`, sin `DT_NEEDED`) | ✅ existe |
| `initramfs/` con `/lib` | ✅ existe |
| Gap: `initramfs/dev/console` no existe | ⚠️ arreglar |
| Gap: `initramfs/init` y `kernel/vmlinuz` trackeados en git | ⚠️ decisión |

---

## 4. Criterios transversales (aplican a TODAS las fases)

1. **Toda compilación se hace en el VPS** (`192.168.1.87`) con el `gcc` del VPS + `--sysroot`/`specs`.
2. **Nada de glibc dentro de la ISO.** Verificación por fase: `ldd` debe mostrar `libmios.so.6`.
3. **Prueba continua en QEMU** tras cada fase — **la ejecuta el usuario**.
4. **Build limpio**: `initramfs/dev/console` debe existir como **dispositivo de caracteres**
   (`sudo mknod initramfs/dev/console c 5 1`), no como fichero normal.
5. Sincronizar Windows ↔ VPS tras cada cambio (`./sync-vps.sh`).
6. Anotar avances en `PROGRESO.md` al cerrar cada fase.

---

## 5. Fases

> Convención de cada fase: **Objetivo · Entregables · Tareas · Hecho cuando · Riesgos.**

### FASE 0 — Fundación dinámica (`libmios.so.6` + `ld-mios.so.1`)

**Objetivo:** convertir lo que hoy es estático dentro de `init` en una librería dinámica propia
+ un cargador propio, y que `init` sea dinámico.

**Entregables**
- `libmios.so.6` (a partir de `src/lib/*.c` + `src/kernel/syscall.c`).
- `ld-mios.so.1` (cargador mínimo).
- `crt1.o` (arranque de programas: `_start` → `main` → `exit`).
- `build.sh` nuevo (genera las 3 cosas + `init` dinámico).

**Tareas**
1. **Organizar símbolos de la libc**: exponer **nombres estándar** (`open`, `read`, `write`,
   `close`, `lseek`…) como API; mantener `mios_*` como alias internos.
2. **Compilar PIC:** todos los objetos de la lib con `-fPIC`.
3. **Enlazar la librería:**
   ```
   gcc -shared -nostdlib -Wl,-soname,libmios.so.6 -fPIC -o libmios.so.6 <objs> -lgcc
   ```
4. **Escribir `crt1.o`** (`src/crt/crt1.S` o `.c`): entrada que llama a `main` y luego a `exit`
   (sin `__libc_start_main` de glibc).
5. **Escribir `ld-mios.so.1`** — el cargador. **Versión mínima (esta fase):**
   - ELF `ET_DYN`, **sin dependencias** (estático).
   - Lee `auxv` (`AT_PHDR`, `AT_PHNUM`, `AT_BASE`, `AT_ENTRY`).
   - Mapea `DT_NEEDED` (por ahora, `libmios.so.6`).
   - Aplica relocalizaciones: `R_X86_64_RELATIVE`, `GLOB_DAT`, `JUMP_SLOT`, `64`.
   - Ejecuta `DT_INIT_ARRAY` / `DT_FINI_ARRAY`.
   - Salta a `AT_ENTRY` del ejecutable.
   - *(TLS viene en fase posterior: ver §6.)*
6. **`build.sh` nuevo:** compilar objetos → `libmios.so.6`; compilar `ld-mios.so.1` y `crt1.o`;
   enlazar `init` con `-pie -Wl,--dynamic-linker=/lib/ld-mios.so.1 -L. -lmios`.
7. **Colocar en `initramfs/`:** `/lib/libmios.so.6`, `/lib/ld-mios.so.1`.
8. **Arreglar el gap** de `initramfs/dev/console`.

**Hecho cuando:** `init` dinámico arranca en QEMU y `ldd init` dice `libmios.so.6`;
el shell de MiOS funciona igual que antes.

**Riesgos:** relocalizaciones mal aplicadas; orden de mapeo; `PT_INTERP` mal escrito.

---

### FASE 1 — Sysroot + `mios-gcc` (wrapper)  ✅ HECHA Y VERIFICADA (2026-10-18)

**Objetivo:** poder compilar **cualquier programa MiOS** desde el VPS con el `gcc` del VPS,
enlazándolo contra `libmios` — sin tocar MiOS.

**Entregables**
- `sysroot/` (`usr/include`, `usr/lib`) — lo monta `mk-sysroot.sh` (artefacto, ignorado en git).
- `mios-gcc`: envoltorio (en vez de `mios.specs`, que es más delicado) — ver nota.
- Primer programa de prueba enlazado dinámicamente contra `libmios`: `examples/hola.c`.

**Tareas**
1. Montar `sysroot/usr/include/` con las cabeceras de MiOS. Hecho: las fuentes viven en
   `include/` (`mios.h`, `mios_types.h`, `stdio.h`, `stdlib.h`, `string.h`, `unistd.h`,
   `fcntl.h`, `errno.h`) y `mk-sysroot.sh` las copia.
2. Copiar a `sysroot/usr/lib/`: `libmios.so.6` (+ symlink `.so`), `ld-mios.so.1`, `crt*.o`.
   Hecho; además se **espeja en `sysroot/lib/`**, que son las rutas que abre el cargador en
   ejecución, de forma que `sudo chroot sysroot /prog` funciona tal cual.
3. Crear `mios.specs`:
   ```
   *link:
   -nostdlib -pie %{!static:--dynamic-linker=/lib/ld-mios.so.1} \
     %{static:-static} <crt1.o> ... -L<mios>/sysroot/usr/lib -lmios -lgcc
   ```
   **No se hizo.** Se optó por el **wrapper** (la alternativa que el propio plan permite):
   `mios-gcc` pasa los flags en la linea de comandos, que es mucho menos fragil que un fichero
   `specs`, y permite decidir segun el caso (`-c`, `-S`, `-E`, `-M` no enlazan). Además el
   ejecutable es **`-no-pie` (ET_EXEC)**, no `-pie`: el cargador de la Fase 0 lo asume así
   (igual que `init`), y `libgcc` se pasa por ruta absoluta porque `--sysroot` tambien afecta a
   las rutas de busqueda de bibliotecas.
4. Crear wrapper `mios-gcc`. Hecho (`mios-gcc`, script bash; se le da `chmod +x` en el build).
5. Compilar un `hola.c` y verificar con `ldd` que pide `libmios.so.6`. Hecho con `readelf -d`
   (`NEEDED libmios.so.6`) y `readelf -l` (`INTERP /lib/ld-mios.so.1`), y **ejecutado** de verdad
   (`hola uno dos`) en el VPS.

**Hecho cuando:** `mios-gcc hola.c -o hola` produce un binario que corre en MiOS
(y también en el VPS, por el truco del ABI, §7). ✅ Verificado el 2026-10-18.

**Extra de esta fase:** para que esto fuera util de verdad hizo falta adelantar parte de la
Fase 2: `src/lib/posix.c` (nombres estandar sobre la API `mios_*`), `errno` via
`__errno_location()` (el cargador aún no implementa `R_X86_64_COPY`), `cons_vprintf()` para
que `printf()` pueda reenviar varargs, y `src/crt/crt1.c` (el `_start` de los programas).

**Riesgos:** `specs` es delicado (evitado usando el wrapper); el `-lgcc` del host sirve al
principio (misma arquitectura). Confirmado en este VPS.


---

### FASE 2 — `libmios` nivel "C / POSIX-lite"  ✅ HECHA Y VERIFICADA (2026-10-18)

**Objetivo:** pasar de ~25 funciones a una libc utilizable por programas reales.

**Desviaciones respecto al plan original (y por qué):**

- `malloc` se apoya en **`brk`**, no en `mmap`: así el montón es contiguo y la fusión de bloques
  vecinos al liberar es siempre correcta (con `mmap` no lo sería).
- El cargador tuvo que aprender **`R_X86_64_COPY`** + **interposición**: `stdin/stdout/stderr` y
  `environ` son símbolos de *datos* que el ejecutable se copia (no valía el truco de "todo por
  función" que se usó con `errno`).
- `errno` sigue siendo `__errno_location()` (ya no por necesidad, sino por gusto): es lo que hace
  glibc.
- `rename` ya existía (syscall) y `tmpfile` se implementa sobre un fichero en `/tmp`.
- `process.c` (fork/exec/wait/señales) se adelantó aquí porque era barato; su uso real (lanzar
  programas desde la shell) es la Fase 4.

| Grupo | Funciones |
|---|---|
| string | `memcpy` `memmove` `memset` `memcmp` `strlen` `strcmp` `strncmp` `strcpy` `strncpy` `strchr` `strrchr` `strstr` `strtok` `strdup` |
| malloc | `malloc` `calloc` `realloc` `free` (sobre `mmap`/`brk`) |
| stdio | **`FILE*`** `fopen` `fclose` `fread` `fwrite` `fseek` `ftell` `fflush` `fprintf` `printf` `snprintf` `sscanf` `fgets` `puts` `putc` `getc` `remove` `rename` `tmpfile` |
| stdlib | `atoi` `strtol` `strtoul` `qsort` `bsearch` `getenv` `setenv` `exit` `abort` `atexit` `__cxa_atexit` `__cxa_finalize` |
| ctype | `isspace` `isdigit` `isalpha` … |
| unistd | `read` `write` `open` `close` `lseek` `dup` `dup2` `pipe` `getpid` `chdir` `getcwd` `unlink` `access` `isatty` `sleep` `usleep` |
| fcntl/stat | `fcntl` `stat` `fstat` `lstat` `chmod` `mkdir` `rmdir` |
| dirent | `opendir` `readdir` `closedir` |
| time | `time` `gettimeofday` `clock_gettime` |
| errno | `errno` global o `__errno_location` (single-thread ⇒ global) |
| misc | `assert` `abort` `__stack_chk_fail` `__stack_chk_guard` |

**Hecho cuando:** programas C no triviales compilan con `mios-gcc` y corren en MiOS.

**Riesgos:** `stdio`/`FILE*` es la parte más pesada; `malloc` depende de `mmap` anónimo.

---

### FASE 3 — `libm` (matemática) + `fenv`  ✅ HECHA Y VERIFICADA (2026-10-18)

**Objetivo:** la librería matemática que necesitan MPFR y `libstdc++`.

**Entregables:** `libm` en `libmios` (o `libmios_m.so`): `sqrt` `pow` `exp` `log` `log2` `log10`
`sin` `cos` `tan` `asin` `acos` `atan` `atan2` `floor` `ceil` `trunc` `round` `fmod` `fabs`
`frexp` `ldexp` `scalbn` `ilogb` `copysign` `nextafter` `hypot` `cbrt` `fmin` `fmax` `nearbyint`
`rint` `lgamma` `tgamma`, **con variantes `float` y `long double`**; `fenv` básico.

**Hecho cuando:** MPFR (Fase 8) y `libstdc++` (Fase 10) enlazan sin símbolos faltantes.

**Riesgos:** `long double` (x87 80-bit) es tedioso; precisión suficiente, no perfecta.

**Como quedó (desviaciones):**

- Todo vive en **`libmios.so.6`** (un solo `.so`); `mk-sysroot.sh` crea `libm.so` y `libc.so` como
  enlaces a ella, así `-lm`/`-lc` enlazan y el `DT_NEEDED` sigue siendo `libmios.so.6`.
- Lo **exacto** se hace con bits o con x87 (`sqrtsd`/`fsqrt`, `frndint` para
  `floor`/`ceil`/`trunc`/`rint`/`nearbyint`); lo **trascendente**, con reducción de argumento +
  polinomios minimax tipo fdlibm o series. `pow` tiene camino exacto para exponente entero.
- Las variantes **`f`** delegan en `double` (redondeo final) y las **`l`** usan x87 donde es exacto
  y `double` en las trascendentes: precisión suficiente, no perfecta (como advierte el plan).
- `fenv` se implementa sobre **x87 + MXCSR** a la vez, para que el modo valga con `double` (SSE) y
  con `long double` (x87).

---

### FASE 4 — Procesos y sistema de ficheros en MiOS  ✅ HECHA Y VERIFICADA (2026-10-18)

**Objetivo:** que MiOS pueda **lanzar y gestionar programas externos**.

**Entregables:** `fork`/`clone`, `execve`, `wait4`, `pipe`, `dup2`, `kill`, `sigaction`/`raise`;
FS en RAM (**tmpfs**) con `/bin`, `/tmp`, `/dev`, `/proc` (mínimo); `PATH`, `chdir`, `getcwd`.

**Hecho cuando:** desde el shell de MiOS se lanza un programa de `/bin` y se recoge su salida.

**Riesgos:** `fork` sin MMU duplicada es complicado; considerar `posix_spawn` primero y dejar
`fork` como stub.

**Como quedó (desviaciones):**

- `fork` **se usa de verdad** (el kernel lo da hecho): no hizo falta ningún sustituto. El shell
  hace `fork()` + `execvp()` y espera con `waitpid()` (`src/core/prog.c`).
- El «FS en RAM» es el propio **initramfs** (rootfs = tmpfs del kernel) más **devtmpfs** en `/dev`
  y **proc/sysfs** montados al arrancar (`src/core/boot.c`). Se añaden `/bin` (programas de
  `userland/`, compilados por `build.sh`) y `/tmp`. No se añadió un tmpfs aparte: el rootfs ya lo es.
- En vez de `posix_spawn` se expone la pareja clásica `fork`/`execvp` y un comando implícito: la
  shell ejecuta como programa cualquier orden que no sea un comando interno.
- Se añadieron `/etc/autorun` (órdenes al arrancar), `cd`/`pwd`, y un `PATH` mínimo por defecto.
- La salida de los programas va a su `stdout` (consola del sistema); MiOS duplica *sus* mensajes al
  puerto serie, pero no puede duplicar la de un programa cualquiera (fd 1 = `/dev/console` = tty0).

---

### FASE 5 — Portar `tcc` (compilador C propio)  ✅ HECHA Y VERIFICADA (2026-10-18)

**Objetivo:** compilador de C **dentro** de MiOS, enlazado contra `libmios`.

**Entregables:** `/bin/tcc` con su **ensamblador y enlazador propios** (no necesita binutils).

**Tareas**
1. Con `mios-gcc` (VPS): compilar las fuentes de `tcc` contra `libmios`. ✅
2. Verificar símbolos que `tcc` pide de libc (~80–150) e implementar los que falten. ✅
   (lo que faltaba: `setjmp`/`longjmp`, `mmap`/`munmap`/`mprotect`, `strtold`)
3. `tcc` necesita `setjmp`/`longjmp`, `alloca`, `mmap`, `dlopen` opcional. ✅
   (`alloca` lo hace `tcc` con su propio código generado; `dlopen` queda fuera y por eso
   se compila con `CONFIG_TCC_STATIC`, que además quita backtrace y `bcheck`)
4. Ajustar `tcc` para emitir ELF con `PT_INTERP=/lib/ld-mios.so.1` y
   `DT_NEEDED=libmios.so.6`. ✅ (`config.h` generado por `tools/build-tcc.sh`;
   con `CONFIG_TCC_ELFINTERP` y `CONFIG_TCC_LIBPATHS`/`CRTPREFIX`, los programas que
   compila piden `-lc` y encuentran `/lib/libc.so` → `libmios.so.6`)

**Hecho cuando:** dentro de MiOS, `tcc -o hola hola.c && ./hola` funciona. ✅
(con la salvedad de que en MiOS no hay `./hola`: el `PATH` por defecto es `/bin:/usr/bin`,
así que se ejecuta con su ruta, `tcc -o /tmp/hola /usr/src/hola.c` y luego `/tmp/hola`)

**Cómo se hizo (resumen):**
- Fuentes: `third_party/tcc` (no versionadas; `tools/fetch-tcc.sh` las baja fijando el tag
  `release_0_9_27`), `tools/build-tcc.sh` genera el `config.h` y compila **en una sola
  unidad** (`tcc.c` con `ONE_SOURCE=1`, que incluye `libtcc.c`).
- `build.sh` pasa a **9 pasos**: el 6 instala en el initramfs `/bin/tcc`,
  `/lib/tcc/{include,libtcc1.a}`, las cabeceras publicas en `/usr/include`, el arranque
  (`/usr/lib/crt1.o`, `crti.o`, `crtn.o`) y los enlaces `/lib/libc.so` y `/lib/libm.so`.
- **Cabeceras publicas portables**: las de argumentos variables usan `va_list` y no
  `__builtin_va_list`, y `math.h` recurre a funciones de libmios cuando el compilador no
  es gcc (tcc no trae los builtins de gcc). Sin esto, tcc no puede ni incluir `<stdio.h>`.
- **`/bin/runlog`**: ejecuta un programa y copia su salida (stdout y stderr) al puerto
  serie; es como se verificó todo esto sin pantalla.
- **Aviso del resultado**: `tcc` se callaba al terminar bien y, al fallar, dejaba los
  errores sueltos. `tools/build-tcc.sh` aplica un parche de tres líneas
  (`tools/tcc-mios-info.inc`) a una **copia** de `tcc.c` en `build/` (las fuentes de
  `third_party/` no se tocan) para que siempre diga cómo ha ido:
  `tcc: compilacion correcta -> <fichero>` o `tcc: la compilacion ha fallado`.

---

### FASE 6b — Editor de texto en consola (`edit`)  ✅ HECHA Y VERIFICADA (2026-10-18)

**Objetivo:** poder **escribir programas dentro de MiOS** (sin depender del VPS) para cerrar
el ciclo escribir → compilar → ejecutar dentro del propio sistema.

**Entregables:** `/bin/edit` (estilo `nano`) + el soporte de `termios` en la libc:
`include/termios.h` y `src/lib/termios.c` (`tcgetattr`, `tcsetattr`, `cfmakeraw`, `tcflush`
sobre el mismo `SYS_ioctl` que ya usaba `isatty`). El editor pone la consola en **modo
crudo** mientras trabaja (sin eco, sin canónico, sin señales: `Ctrl-S`/`Ctrl-Q` llegan como
teclas), pinta con códigos ANSI sobre el terminal virtual del kernel y deja la consola como
estaba al salir.

**Verificación:** además de la prueba a mano del usuario, el editor trae dos modos
automáticos que no necesitan consola interactiva y que se pueden lanzar desde `runlog` o
`/etc/autorun`: `edit --selftest` (motor de edición: 17 comprobaciones) y `edit --ttytest`
(ioctl de `termios` y modo crudo). Ambos dan `0 fallo(s)` en el *chroot* del sysroot y en la
VM (log serie).

---

### FASE 6 — Self-hosting con `tcc` ⭐  ✅ HECHA Y VERIFICADA (2026-10-18)

**Objetivo:** que `tcc` **se compile a sí mismo dentro de MiOS**.

**Hecho cuando:** **MiOS se compila a sí mismo en C** (self-hosting C conseguido). ✅

**Cómo se hizo:**
- Las **fuentes de `tcc` van dentro de la ISO** (`/usr/src/tcc/`, copiadas por
  `tools/build-tcc.sh`: los `.c`/`.h`/`.def` de la raíz, más `lib/` e `include/`). Ahí está
  también el `config.h` generado, que es lo único que tcc ha necesitado para adaptarse a MiOS.
- Con `ONE_SOURCE` (por defecto en `tcc.h`) basta **un fichero**: `tcc.c` incluye `libtcc.c`, y
  `libtcc.c` incluye el resto (preprocesador, generador, ELF, ensamblador, `x86_64-gen/link`…).
  Así que dentro de MiOS la orden es simplemente:
  `tcc -o /tmp/tcc2 /usr/src/tcc/tcc.c`.
- **Correccion importante en `libtcc1.a`**: la `stdarg.h` de tcc para x86_64 manda
  `va_start`/`va_arg` a `__va_start`/`__va_arg`, que viven en `lib/va_list.c`. Sin ese objeto,
  compilar tcc se quedaba a un paso del final: `tcc: error: undefined symbol '__va_start'`.
  `libtcc1.a` se construye ahora con el juego completo de x86_64 del `lib/Makefile` de tcc
  (`libtcc1.o`, `va_list.o`, `alloca86_64.o`, `alloca86_64-bt.o`), sin `bcheck.o` (el modo de
  comprobación de límites está apagado por `CONFIG_TCCBOOT`).

**Verificación (log serie, ISO real):**
`runlog tcc -o /tmp/tcc2 /usr/src/tcc/tcc.c` → `codigo 0`;
`runlog /tmp/tcc2 -o /tmp/hola2 /usr/src/hola.c` → `codigo 0`;
`runlog /tmp/hola2` → `codigo 0` con sus seis comprobaciones `OK` (`resultado: 0 fallo(s)`).
Es decir: `tcc` compila `tcc` dentro de MiOS y **el compilador resultante compila y ejecuta un
programa**.

---

### FASE 7 — `binutils` (as, ld) propias  ✅ HECHA (2026-10-19)

**Objetivo:** ensamblador y enlazador GNU-compatibles sobre `libmios` (necesarios para GCC).

**Entregables:** `/bin/as`, `/bin/ld` enlazados contra `libmios`; `ld` con
`--dynamic-linker=/lib/ld-mios.so.1`.

**Hecho cuando:** `as`/`ld` corren en MiOS y ensamblan/enlazan un `.o` real.

> Nota: `tcc` ya trae as/ld internos. Esta fase es **para GCC**, no para tcc.

**Resultado:** binutils **2.42** (`tools/build-binutils.sh`, `configure` nativo con `mios-gcc`;
arbol de trabajo sin espacios en `~/mios-btl/`). Lo que hubo que resolver por el camino:
**TLS estático** en el cargador (`src/ld/ldmios.c`: `PT_TLS`, reparto *local-exec* con el bloque del
ejecutable acabando en el puntero de hilo, TCB y `arch_prctl(ARCH_SET_FS)`) — sin él `bfd_init()`
morría leyendo `%fs`; y **`realpath()`/`canonicalize_file_name()`** en la libc, sin las cuales
`lrealpath()` de libiberty se queda sin implementación y devuelve basura (era el otro SIGSEGV, en
`ld`). Con eso, `as`/`ld` se ejecutan en el VPS, en un `chroot` del sysroot y **dentro de MiOS**
(VM: ensambla, enlaza y el programa sale `as y ld de MiOS: hola`).

**Pendiente opcional:** `ar`/`nm`/`objdump`/`strip` (`make install-binutils`) y TLS dentro de una
`.so` (`R_X86_64_TPOFF64`/`__tls_get_addr`), que GCC todavía no pide.

---

### FASE 8 — `GMP` + `MPFR` + `MPC`  ✅ HECHA (2026-10-19)

**Objetivo:** las librerías aritméticas del *front-end* de GCC.

**Entregables:** `libgmp.so`, `libmpfr.so`, `libmpc.so` sobre `libmios` (vienen dentro de GCC).

**Hecho cuando:** las tres enlazan y pasan un test de cálculo.

**Riesgos:** MPFR exige `libm` razonablemente completa (por eso va después de Fase 3).

**Resultado:** GMP **6.2.1**, MPFR **4.1.0** y MPC **1.2.1** (las versiones exactas que pide
`contrib/download_prerequisites` de GCC 13.3; `tools/fetch-gcc-libs.sh` +
`tools/build-gcc-libs.sh`, espejo sin espacios en `~/mios-gcl`) instaladas **como bibliotecas
compartidas** en `/lib` y cabeceras en `/usr/include`, enlazadas contra `libmios` y cargadas por
nuestro cargador **dentro de MiOS**. Lo que hubo que resolver:

- **`DT_GNU_HASH` en el cargador**: solo entendía `DT_HASH` (lo que lleva `libmios`), pero lo que
  enlaza el `gcc` del VPS lleva `--hash-style=gnu`; sin la tabla GNU su `nsym` era 0 y *todos* los
  símbolos de GMP/MPFR/MPC se resolvían a 0 (salto a la dirección 0).
- **`DT_NEEDED` recursivo** (`libmpc` → `libmpfr` → `libgmp`), sin cargar dos veces la misma
  biblioteca, y búsqueda global sin símbolos `STB_LOCAL`.
- **Símbolo sin resolver = mensaje con su nombre** (`ld-mios: simbolo sin resolver: <nombre>`),
  en vez de un SIGSEGV mudo.
- **`mios-gcc -shared`**: sin `crt1.o`/`-_start`/`--dynamic-linker` y con `-fPIC` (lo que llaman
  los `libtool` de esta gente).
- **MPFR `--disable-thread-safe`**: MPFR 4.x usa TLS para sus cachés y el cargador aún no
  implementa TLS dentro de una `.so`; MiOS es mono-hilo, así que global = equivalente.
- **`<stdio.h>` define `_STDIO_H`**: GMP reconoce `stdio.h` por la guarda de glibc y sin esa marca
  escondía todos sus prototipos con `FILE` (`mpz/inp_str.c` no compilaba).

**Verificación:** `examples/gmptest.c` (100!, pi, sqrt(2), exp(sqrt 2), producto complejo) da
0 fallos en el VPS, en el `chroot` del sysroot y **dentro de MiOS** (VM headless, `runlog`,
código 0); `./build.sh` (ahora 11 pasos) deja las tres en la ISO.

**Pendiente opcional:** TLS dentro de una `.so` (`R_X86_64_TPOFF64`/`__tls_get_addr`), que nadie
pide todavía, y `ar`/`nm`/`objdump`/`strip` (que la Fase 9 agradecerá).

---

### FASE 9 — `GCC` etapa C

**Objetivo:** GCC que compila **C**, corriendo sobre `libmios`.

**Tareas.** Configurar GCC con la **lista mínima**:
```
--enable-languages=c
--disable-bootstrap --disable-nls --disable-werror
--disable-libsanitizer --disable-libgomp --disable-libitm
--disable-libquadmath --disable-libssp --disable-libvtv
--with-sysroot=<sysroot>
```
Construir con `mios-gcc` → binarios que corren en MiOS. Implementar los símbolos de libc que GCC
pida de más.

**Hecho cuando:** dentro de MiOS, `gcc -o hola hola.c && ./hola` funciona.

**Riesgos (los más duros):** **TLS** (`%fs`, `__tls_get_addr`) en el cargador; **`dl_iterate_phdr`**
para el *unwinder*; señales y `setjmp` sólidos; consumo de memoria.

**Prerrequisitos y hallazgos (2026-10-19, antes de empezar):**

- **GCC está escrito en C++** desde 4.8, así que para compilar sus fuentes hace falta un
  **`g++` en el VPS** (`cc1plus` + cabeceras de libstdc++) — el VPS no lo tiene instalado
  (`apt-cache policy g++` da candidato, hace falta `sudo apt-get install -y g++`); el *build* puede
  pedir además `texinfo`/`gperf` (el binario `makeinfo` sí está).
- **Buenísima noticia:** un `cc1` real **no enlaza libstdc++**. `nm -D --undefined-only` sobre el
  `cc1` de Ubuntu da **0** símbolos `std::` (`_ZSt`/`_ZNS`) y solo uno del estilo `_Znwm`/`__cxa*`
  (de los 514 sin resolver, el resto es libc). Es decir: basta un **runtime C++ mínimo propio** en
  libmios (`operator new`/`delete`, `__cxa_atexit`, `__cxa_guard_*`, `std::terminate`,
  `__cxa_pure_virtual`) — **sin** portar la STL, que es lo caro de verdad. La Fase 10 (libstdc++)
  sigue siendo para el *front-end* de C++, no para construir `cc1`.
- Ese `cc1` necesita también **libisl** (solo Graphite: si no hay `isl/*.h` en el VPS —no los hay—
  GCC se configura sin Graphite, que es lo que queremos), **libz** (GCC trae su propia zlib, que se
  construye con `mios-gcc` en el mismo *build*) y **libzstd** (opcional).
- **Espacio:** el árbol de fuentes + build de GCC pide ~2 GB; al VPS le quedan ~2.4 GB (el usuario
  ampliará el disco antes de empezar). Conviene además `-g0` en `CFLAGS` (GCC se construye con `-g`
  por defecto y eso infla mucho los objetos).
- `tools/fetch-gcc.sh` ya está escrito (baja **GCC 13.3.0**, la versión que trae Ubuntu 24.04 y la
  que espera binutils 2.42 + GMP/MPFR/MPC instalados en la Fase 8). El **espejo importa**:
  `ftp.gnu.org` iba a 11 KB/s desde el VPS (2 h de descarga) y `mirrors.kernel.org` va normal, así
  que el script prueba primero el segundo.
- Plan de ataque: `mios-g++` (envoltorio: `--sysroot`, `-nostdlib`, sin libstdc++, con el runtime
  mínimo), `src/lib/cxx` en libmios, *mirror* sin espacios de las fuentes (~`$HOME/mios-gcc`),
  `configure` con `--with-gmp/mpfr/mpc` apuntando al prefijo de la Fase 8, y primero **`make
  all-gcc`** (cc1 + driver) antes de las bibliotecas del *target* (`libgcc`).

---

### FASE 10 — `libstdc++` + `GCC` etapa C++

**Objetivo:** GCC con **C++** sobre `libmios` (aquí entra libstdc++).

**Tareas:** `--enable-languages=c,c++` + `--enable-shared`; compilar `libstdc++` contra `libmios`
(necesita `libm`, `locale` mínimo, `wchar`); resolver **pthread** (stubs single-thread o hilos
reales con `clone`); **excepciones** (`libgcc_s` + `dl_iterate_phdr` + `.eh_frame`);
`__cxa_atexit` / `__cxa_finalize` / `__dso_handle` en libmios.

**Hecho cuando:** dentro de MiOS, `g++ -o hola hola.cpp && ./hola` funciona
(con `std::vector`, `std::string`, `iostream`).

**Riesgos:** los más altos: excepciones, TLS, hilos, locale.

---

### FASE 11 — Bootstrap nativo y self-hosting completo

**Objetivo:** que GCC **se reconstruya a sí mismo dentro de MiOS**.

**Tareas:** **stage2/stage3** — usar el GCC de la Fase 9/10 para recompilar GCC dentro de MiOS;
comparar el binario resultante; opcionalmente reconstruir también binutils y libmios.

**Hecho cuando:** **self-hosting completo** — MiOS compila su propio compilador, su propia libc
y su propio sistema.

---

## 6. Ingeniería fina (transversal, por orden de aparición)

| Tema | Cuándo | Qué implica |
|---|---|---|
| Relocalizaciones básicas | Fase 0 | `RELATIVE`, `GLOB_DAT`, `JUMP_SLOT`, `64` |
| `DT_INIT_ARRAY` / `FINI_ARRAY` | Fase 0 | ejecutar constructores/destructores |
| `__cxa_atexit` / `__cxa_finalize` | Fase 2 | destructores C++ y `atexit` |
| `mmap` anónimo | Fase 2 | base de `malloc` y de GCC |
| `__stack_chk_fail` / guard | Fase 2 | o compilar con `-fno-stack-protector` |
| **TLS** (estático) | Fase 9 | TCB, `%fs`, `R_X86_64_TPOFF64`, `__tls_get_addr` |
| `dl_iterate_phdr` | Fase 9-10 | unwinder de excepciones |
| Unwinding (`libgcc_s`, `.eh_frame`) | Fase 10 | excepciones C++ |
| `pthread` (real o stub) | Fase 10 | `std::thread`, mutex |
| `locale` / `wchar` | Fase 10 | `std::locale`, streams |

---

## 7. El truco del ABI (por qué se puede probar en el VPS)

> **MiOS corre sobre el kernel Linux x86_64. Por tanto un "binario MiOS" (ELF con
> `PT_INTERP=/lib/ld-mios.so.1` y `DT_NEEDED=libmios.so.6`) se ejecuta DIRECTAMENTE en el VPS**,
> colocando `ld-mios.so.1` y `libmios.so.6` donde el binario los espera.

| Tarea | Dónde | ¿Necesita fork/exec/FS de MiOS? |
|---|---|---|
| **Construir** el toolchain | VPS | ❌ no |
| **Probar** los binarios | VPS (o QEMU) | ❌ no |
| **Ejecutar** dentro de MiOS | MiOS | ✅ sí (Fase 4) |

---

## 8. Entorno de build (VPS) y sincronización

**Dependencias en Ubuntu**
```
build-essential cpio gzip grub-pc-bin grub-common xorriso mtools \
qemu-system-x86 inotify-tools git
```
(para GCC, más adelante: `g++` (¡GCC está escrito en C++!), `flex bison texinfo gawk gperf`; el
`g++` trae el `cc1plus` y las cabeceras de libstdc++, que son lo que falta en el VPS)

**Sincronizar Windows ↔ VPS**
```
./sync-vps.sh
```
Destino: `chichimokers@192.168.1.87:"/home/chichimokers/OS Nuevo test"`.

---

## 9. Riesgos globales y mitigaciones

| Riesgo | Impacto | Mitigación |
|---|---|---|
| `stdio`/`FILE*` complejo | retrasa Fase 2 | implementar por capas, con tests |
| `fork` sin MMU | complica Fase 4 | priorizar `posix_spawn`; `fork` después |
| **TLS** en el loader | bloquea GCC | TLS estático single-thread |
| Excepciones C++ / unwinder | bloquea Fase 10 | `libgcc_s` + `dl_iterate_phdr` |
| `long double` (x87) en libm | retrasa Fase 3 | precisión suficiente, no exacta |
| Alcance temporal | desánimo | hitos visibles (Fase 6 = tcc funciona) |

---

## 10. Hitos y expectativa temporal

| Hito | Fase | Estimación |
|---|---|---|
| `init` dinámico + loader | 0 | semanas |
| Compilar contra `libmios` con `mios-gcc` | 1 | semanas |
| libc usable (stdio/malloc/procesos) | 2-4 | 2-4 meses |
| **`tcc` dentro de MiOS** ⭐ | 5-6 | ✅ hecho (2026-10-18) |
| *Escribir* código dentro de MiOS (`edit`) | 6b | ✅ hecho (2026-10-18) |
| `binutils` (`as`, `ld` 2.42) dentro de MiOS | 7 | ✅ hecho (2026-10-19) |
| `GMP`/`MPFR`/`MPC` | 8 | +1–2 meses |
| **`GCC`-C dentro de MiOS** | 9 | ~8–12 meses |
| **`GCC`-C++ + self-hosting** | 10–11 | ~12+ meses |

---

## 11. Orden de arranque recomendado (lo inmediato)

```
1. Sincronizar Windows → VPS:  ./sync-vps.sh   (o sync-vps.bat)
2. En el VPS: ./build.sh  (11 pasos; comprobar que termina limpio)
3. Arrancar la ISO en la VM y leer el log serie (lo nuevo de cada fase).
4. FASE 9 (lo siguiente): un GCC que compile C sobre `libmios`.
   - Configurar GCC 13.3 con la lista mínima (`--enable-languages=c`, `--disable-bootstrap`,
     `--disable-nls`, sin libsanitizer/libgomp/libquadmath..., `--with-gmp/mpfr/mpc` apuntando al
     prefijo de la Fase 8) y construirlo con `mios-gcc`.
   - Esperar huecos nuevos de libc (`dl_iterate_phdr`, señales, `setjmp`) e irlos tapando con
     tests: cada símbolo que falte lo dice ahora el cargador por su nombre.
   - Verificar con `gcc -o hola hola.c` **dentro de MiOS**.
5. Anotar todo en PROGRESO.md.
```

---

## 12. Definición de "éxito"

- **Éxito C (Fase 6):** `tcc` compila programas C y se compila a sí mismo dentro de MiOS.
- **Éxito total (Fase 11):** `g++` compila C++ dentro de MiOS y GCC se reconstruye a sí mismo —
  **self-hosting completo**, con `libmios` y `ld-mios.so.1` propios, y `libstdc++` como único
  componente GNU.

---

*Fin del plan.*
