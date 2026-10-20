# AGENTS.md

MiOS is a **PID 1 (`init`) written in C, static and freestanding, with no libc**, running on the
Linux x86_64 kernel and packaged as a GRUB ISO. It is not a kernel or a distro: it is a userspace
`init` + own shell + 34 commands, all talking to the kernel through raw syscalls.

## VPS, sincronización y bitácora (reglas del proyecto)

- **Todo se compila y se prueba en la máquina Linux `192.168.1.87`** (usuario `chichimokers`,
  contraseña `123`). Windows es solo el editor.
- **Tras cada cambio, sincroniza** el repo con el VPS para que ambos queden iguales:
  `./sync-vps.sh` (Linux/Git Bash) o **`sync-vps.bat`** (Windows, doble clic). Destino:
  `chichimokers@192.168.1.87:"/home/chichimokers/OS Nuevo test"` (la carpeta tiene espacios, va entre comillas).
- **La sincronización es en los dos sentidos**: `sync-vps.bat` primero **sube** la carpeta al VPS y
  después **trae** del VPS lo que haya cambiado allí (los artefactos del build: `initramfs/init`,
  `initramfs/bin`, `initramfs/lib`, `initramfs/usr`, `iso/...`), así Windows no se queda con una
  copia vieja de lo compilado. Se sube **antes** de traer, para no pisar ningún cambio local sin
  subir. Solo traer (sin subir): **`pull-vps.bat`**. No se traen (a propósito): `third_party/`
  (fuentes que bajan los `tools/fetch-*.sh`), `build/`, `sysroot/`, `mios-bin/`, `.git/`,
  `MiOS.iso` (para la ISO está `get-iso.bat`: no se debe pisar mientras la VM la tiene montada) y
  `*.cpio.gz`. Ojo: lo que se borre en Windows y siga en el VPS **vuelve** al traer.
- **Al subir no se envían los artefactos generados** (`initramfs/init`, `initramfs/bin`,
  `initramfs/usr`, `initramfs/lib/tcc`, `initramfs/lib/*.so*` y `ld-mios.so.1`): los produce el VPS
  y subirlos de vuelta sustituiría sus enlaces (`libc.so` → `libmios.so.6`,
  `libgmp.so` → `libgmp.so.10.4.1`) por copias del fichero entero. Windows los tiene solo como
  espejo de lo compilado (los trae `pull-vps.bat`).
- **El push devuelve el bit `+x`** a los `*.sh`, a `tools/*.sh` y a `mios-gcc` en el VPS: el `tar`
  de Windows no lo conserva y, sin él, `build.sh` y `mios-gcc` fallan con *Permission denied*.
- **Acceso sin contraseña ya configurado**: hay una clave SSH (`~/.ssh/id_rsa`) autorizada en el VPS,
  así que `ssh`/`scp`/`sync-vps.bat` no piden la `123`. Si alguna vez falta, `sync-vps.bat` cae al
  respaldo con contraseña vía `SSH_ASKPASS`. (`plink`/`pscp` están instalados pero **no** se usan.)
- **Pruebas dentro del SO:** la consola interactiva de MiOS la usa **el usuario a mano** (pídeselo).
  El asistente **sí puede** arrancar la VM sin ventana y leer el **log serie** de la VM para
  autoverificar el arranque y diagnosticar (ver «Ciclo de verificación» abajo). Así se encontró y
  arregló el primer *kernel panic* de la Fase 0.
- **Scripts para el usuario:** `sync-vps.bat` (sincronizar en los dos sentidos), `pull-vps.bat`
  (solo traer del VPS), `build-vps.bat` (sync + build en el VPS + traer la ISO), `get-iso.bat`
  (solo traer la ISO), `run-vm.bat` (arrancar la VM de MiOS) y `mios-cc.bat` (compilar un `.c` con
  `mios-gcc` y traer el binario a `mios-bin/`).
- **Bitácora obligatoria:** al cerrar cada fase, añade una entrada en `docs/PROGRESO.md` (fecha,
  fase, hecho, falta, verificación, bloqueos) y actualiza la tabla de estado.
- El plan completo del toolchain (tcc → GCC) está en `docs/PLAN-TOOLCHAIN.md`.

## Ciclo de verificación (VMware + log serie)

La VM de MiOS en este PC es `C:\Users\erne\Documents\Virtual Machines\Ubuntu\Ubuntu.vmx`
(`memsize=4096`; CD = `C:\Users\erne\Downloads\MiOS.iso`; serie a fichero =
`C:\Users\erne\mioos.log`). La ISO ya arranca con `console=ttyS0,115200 console=tty0`, así que
**los mensajes del kernel y los kernel panics se copian a `mioos.log`** y se pueden leer como fichero.

Ciclo rápido del asistente (sin ver la pantalla):

```powershell
$vmrun = 'C:\Program Files (x86)\VMware\VMware Workstation\vmrun.exe'
$vmx   = 'C:\Users\erne\Documents\Virtual Machines\Ubuntu\Ubuntu.vmx'
Remove-Item 'C:\Users\erne\mioos.log' -Force -ErrorAction SilentlyContinue
& $vmrun -T ws start $vmx nogui      # arranca sin ventana
Start-Sleep -Seconds 25              # deja arrancar MiOS
& $vmrun -T ws stop  $vmx hard       # apaga
Get-Content 'C:\Users\erne\mioos.log' # banner MiOS + prompt => OK
```

- **No** sobrescribas `C:\Users\erne\Downloads\MiOS.iso` mientras la VM la tenga montada.
- Si el log acaba en `Kernel panic ... Attempted to kill init`, el fallo es del cargador o del
  initramfs; los `traps: init[1] ... in libmios.so.6[...]` apuntan al propio cargador.
- **Atajo sin VM para probar el cargador (¡PELIGROSO — leer antes!):** copiar `initramfs/{init,lib/*}`
  a `/tmp/mt` en el VPS y `sudo chroot /tmp/mt /init` (es el mismo kernel x86_64/ABI Linux).
  **Pero MiOS monta `proc`/`sysfs`/`devtmpfs`, y `devtmpfs` es un *singleton* (todos sus montajes
  comparten superbloque): si luego se borra el arbol con los montajes activos, `rm -rf /tmp/mt` como
  root borra nodos del `/dev` REAL** (paso el 2026-10-18: dejo `/dev/null` como fichero normal y sin
  `/dev/zero`, `/dev/random`, `/dev/urandom`..., y el `sshd` del VPS dejo de aceptar conexiones —
  hubo que reiniciar la maquina). Hazlo siempre en un namespace de montaje propio y desmonta antes
  de borrar:

  ```bash
  sudo unshare -m --propagation private bash -c '
      mount -t tmpfs tmpfs /mnt || exit 1
      mkdir -p /mnt/mt && cp -a /tmp/mt/. /mnt/mt/
      chroot /mnt/mt /init
      rc=$?
      umount -R /mnt/mt 2>/dev/null; umount /mnt 2>/dev/null
      exit $rc'
  ```

  Si aun asi se rompe `/dev`, se arregla con:
  `sudo mknod -m 666 /dev/null c 1 3` (y revisar `/dev/zero` 1:5, `/dev/random` 1:8,
  `/dev/urandom` 1:9, `/dev/console` 5:1, `/dev/tty` 5:0, `/dev/ptmx` 5:2).

## Build & run

- Build requires **Linux/bash**. The repo is often edited on Windows, but nothing here builds on
  Windows (bash scripts + `grub-mkrescue` + `cpio`). Build on the Linux VPS above.
- `./build.sh` — **11 steps**: builds the three artefacts, assembles the **sysroot**, compiles the
  `/bin` programs, builds **`tcc`** (Fase 5), **binutils** (Fase 7) and **GMP/MPFR/MPC** (Fase 8),
  packs the initramfs and writes `MiOS.iso`:
  1. `libmios.so.6` (own libc, PIC) from `src/lib/*.c` + `src/kernel/*.c`;
  2. `ld-mios.so.1` (own dynamic loader, static) from `src/ld/ldmios.c`;
  3. `init` (dynamic ELF, `PT_INTERP=/lib/ld-mios.so.1`, `DT_NEEDED=libmios.so.6`) from
     `src/core/`, `src/fs/`, `src/net/`, linked against `libmios`;
  4. `mk-sysroot.sh` — the sysroot must exist before compiling anything with `mios-gcc`;
  5. compile `userland/*.c` with `mios-gcc` into `initramfs/bin/` (the programs the shell launches);
  6. `tools/build-tcc.sh` — compile `tcc` with `mios-gcc` and install it **inside the initramfs**:
     `/bin/tcc`, `/lib/tcc/{include,libtcc1.a}`, the public headers in `/usr/include`, the crt
     objects in `/usr/lib` (`crt1.o`, `crti.o`, `crtn.o`) and `/lib/libc.so`+`/lib/libm.so` →
     `libmios.so.6`. It also patches a copy of `tcc.c` so the compiler reports the result of a
     compile, and ships the patched sources as `/usr/src/tcc/`. If `third_party/tcc` is missing the
     step warns and continues (MiOS itself does not need `tcc` to boot);
  7. `tools/build-binutils.sh` — configure + build **binutils 2.42** with `mios-gcc` (Fase 7) and
     install `as`/`ld` in `initramfs/bin/` (`--no-install` stops after building and testing). Real
     GNU `as`/`ld` are what GCC expects, so this phase must produce working tools, not a stopgap.
     Needs `make`, `bison`, `flex`, `texinfo` and the binutils sources
     (`tools/fetch-binutils.sh`); non-lethal if missing (they are not needed to boot). The script
     is **incremental** (it reuses `$HOME/mios-btl/build`); after adding a function to the libc run
     it with `--reconfigure`, which deletes binutils' `config.cache` and repeats every feature
     probe (`HAVE_*`) — without that, a newly added function stays "no" forever. `--clean` throws
     the tree away;
  8. `tools/build-gcc-libs.sh` — configure + build **GMP 6.2.1, MPFR 4.1.0 and MPC 1.2.1** with
     `mios-gcc` (Fase 8; the versions GCC 13.3 pins in its `contrib/download_prerequisites`,
     fetched by `tools/fetch-gcc-libs.sh`) and install them as **shared libraries** in
     `initramfs/lib/` + headers in `initramfs/usr/include/` (`--no-install` stops after building and
     testing). Same parking-lot trick as binutils (`$HOME/mios-gcl`, plus a `cc` wrapper). MPFR is
     built `--disable-thread-safe` on purpose: MPFR 4.x otherwise uses `__thread` caches and the
     loader does not implement TLS *inside a `.so`* (`R_X86_64_TPOFF64`/`__tls_get_addr`) yet — MiOS
     is single-threaded, so the global caches are equivalent. `--reconfigure`/`--clean` as above;
     `examples/gmptest.c` is the test (100!, pi, sqrt(2), exp, a complex product), run on the VPS, in
     the sysroot chroot and inside MiOS;
  9. install `libmios.so.6` + `ld-mios.so.1` into `initramfs/lib/` (plus the `libmios.so` symlink
     that `ld -lmios` looks up);
  10. pack `initramfs/` into a cpio.gz and `grub-mkrescue` → `MiOS.iso`;
  11. smoke test: compile `examples/hola.c` with `mios-gcc`.
  There is **no Makefile and no incremental build**; intermediates go to `build/` (gitignored).
- **Writing your own MiOS programs:** `./mios-gcc prog.c -o prog` (a bash wrapper around the VPS
  `gcc`). It uses `--sysroot=sysroot`, `-ffreestanding -fno-builtin -fno-pie`, and links
  `crt1.o` + `-lmios` + `-Wl,--dynamic-linker=/lib/ld-mios.so.1`. Programs are **ET_EXEC
  (non-PIE)** for now, like `init`. From Windows use `mios-cc.bat prog.c [name] [run]`, which
  compiles on the VPS and drops the binary in `mios-bin/`.
- `mk-sysroot.sh` builds `sysroot/` (gitignored): `usr/include` (from `include/`) + `crt1.o` +
  `libmios.so.6` + `ld-mios.so.1`, mirrored into `sysroot/lib/`. The sysroot doubles as a runnable
  root, so a program can be tested on the VPS **without touching the host**:
  `cp prog sysroot/ && sudo chroot sysroot /prog` (safe: these programs mount nothing).
  For the public header layout see `include/` (`mios.h` is the umbrella); the internal
  `src/mios.h` is a different file.
- The loader assumes `init` is **ET_EXEC** (non-PIE, load bias 0) and is itself placed at
  `0x40000000` via `-Wl,-Ttext-segment` to avoid clashing with `init` at `0x400000`. It also
  honours `DT_RPATH`/`DT_RUNPATH` and `LD_LIBRARY_PATH` before falling back to `/lib`.
- Library loading follows `DT_NEEDED` **recursively** (a library may need one the executable never
  names: `libmpc` → `libmpfr` → `libgmp`) and never loads the same one twice (`DT_SONAME` or file
  name). Symbol lookup goes over all loaded modules (`STB_LOCAL` symbols are skipped, as they
  cannot be interposed). It understands both hash tables: **`DT_HASH`** and **`DT_GNU_HASH`** —
  the latter is essential, because anything linked by the VPS `gcc` (GMP/MPFR/MPC get
  `--hash-style=gnu` by default, `libmios` is built `--hash-style=sysv`) has *no* `DT_HASH`, and
  without it their symbol table looked empty and *every* symbol they exported resolved to 0
  (a jump to address 0). An unresolved non-weak symbol now prints
  `ld-mios: simbolo sin resolver: <name>` instead of silently jumping to 0.
- The loader also prepares **static TLS** (variables `__thread`): it reads `PT_TLS`, puts the
  executable's block ending exactly at the thread pointer, follows it with the libraries' blocks,
  writes the TCB self-pointer and loads `%fs` with `arch_prctl(ARCH_SET_FS)`. It is the
  `local-exec` model gcc emits for non-PIC executables; without it any `%fs:` access faults (that
  is how `as`/`ld` used to die inside `bfd_init`). `R_X86_64_TPOFF64`/`__tls_get_addr` (TLS inside
  a `.so`) are **not** implemented yet — nothing needs them so far.
- **Running MiOS binaries *on the VPS* (host-run):** the ported tools (binutils' `configure`, and
  later GCC's) have to *execute* what they build. `tools/mios-host-run.sh --create` (needs `sudo`)
  links `/usr/lib/ld-mios.so.1` + `/usr/lib/libmios.so.6` to the sysroot, so a normal MiOS binary
  (`INTERP=/lib/ld-mios.so.1`) runs as-is on the host; `--remove` undoes it. `mios-gcc`'s
  `MIOS_HOST_RUN=1` is the alternative (bakes absolute sysroot paths into the ELF).
- **binutils needs a path without spaces**: it rejects a source/build tree under `"OS Nuevo test"`,
  so `tools/build-binutils.sh` mirrors `third_party/binutils` into `$HOME/mios-btl/` (`src`,
  `build`, `inst`, plus a `cc` wrapper that calls `mios-gcc`). Configure is **native**
  (`host=build=target=x86_64-linux-gnu`, `CC=mios-gcc`) so the tools get plain names `as`/`ld`.
- Deps (from README): `gcc make binutils cpio gzip grub-pc-bin grub-common xorriso mtools`;
  `inotify-tools` only for `watch.sh`. GMP/MPFR/MPC's `configure` also wants `m4`, and binutils
  wants `bison flex texinfo`. Fase 9 (GCC) will want **`g++`** (GCC is written in C++, and the VPS
  has no `cc1plus` yet) plus `gperf`/`texinfo`.
- Run: `qemu-system-x86_64 -m 1G -cdrom MiOS.iso` (GRUB entry "MiOS" auto-boots).
- `./watch.sh` auto-reruns `build.sh` on changes to `src/` (only `.c/.cpp/.h/.hpp`).

### Build gotchas (will bite you)

- `build.sh` **aborts unless `initramfs/dev/console` exists**, but it is **not tracked** in git (the
  `initramfs/dev/` dir is empty, and git cannot store device nodes). On a fresh clone create it as
  a **char device** first: `sudo mknod initramfs/dev/console c 5 1`. Do **not** `touch` it — a
  regular file makes the kernel open the wrong stdout and MiOS prints nothing.
- `initramfs/init` is a **committed build artifact** (unlike the ISO). Running `./build.sh`
  overwrites a tracked file, so expect git diffs after every successful build. Both `sync-vps.*`
  scripts **exclude `initramfs/init`**: the Windows copy is stale and would clobber the freshly
  built one on the VPS (and mislead `strings initramfs/init` checks). Same reason `MiOS.iso` and
  `*.cpio.gz` are excluded — and why a sync must always come **before** a build, never after.
- **Los permisos del initramfs importan:** `init` y `lib/ld-mios.so.1` deben ir en **755**. `cp -f`
  conserva el modo del destino, asi que un `ld-mios.so.1` que llegue en 0664 (p. ej. desde Windows,
  cuyo `tar` no trae el bit `+x`) deja la ISO con `Failed to execute /init (error -13)` +
  `Kernel panic - not syncing: No working init found`: el kernel no puede ejecutar el interprete.
  `build.sh` lo sanea antes de empaquetar, y el push devuelve el `+x` a `*.sh`/`mios-gcc` en el VPS.
- **A program's stdout is tty0 (VGA), not the serial port.** `src/lib/console.c` writes MiOS's own
  messages to fd 1 *and* mirrors them to `/dev/ttyS0` (so the shell banner/prompt show up in
  `mioos.log`). A child program's `printf` only goes to fd 1 = `/dev/console` = tty0. So headless
  verification of a launched program uses the shell's own report (e.g. `MiOS: 'args' termino con
  codigo N`, which is a `cons_*` message and does reach the serial log), not the program's output.
  To get a program's output into the serial log it must write to `/dev/ttyS0` or use `cons_*`.
  The easy way is `/bin/runlog` (`userland/runlog.c`): it runs another program with a pipe as its
  stdout/stderr and copies everything it prints to **both** the screen and `/dev/ttyS0` —
  so `runlog tcc -o /tmp/hola /usr/src/hola.c` puts `tcc`'s diagnostics in the serial log.
- Ignored (generated): `*.iso`, `*.cpio.gz`, `third_party/` (tcc + binutils + gmp/mpfr/mpc + gcc
  sources; `tools/fetch-tcc.sh`, `tools/fetch-binutils.sh`, `tools/fetch-gcc-libs.sh`,
  `tools/fetch-gcc.sh`),
  `initramfs/{bin/,lib/libmios.so.6,lib/ld-mios.so.1,lib/tcc/,lib/libgmp.so*,lib/libmpfr.so*,
  lib/libmpc.so*,usr/,tmp/}`, `build/`, `sysroot/`,
  `mios-bin/`. Tracked large assets: `iso/boot/vmlinuz` and the
  `initramfs/lib/modules/**/*.ko`.

## No tests, linters, or CI

Verification = a clean `./build.sh` on the VPS (10 steps) + boot the ISO in the VM (the assistant
does this headlessly and reads `mioos.log`; the interactive shell is exercised by the user).
Compiled MiOS programs can also be run on the VPS inside the sysroot chroot
(`sudo chroot sysroot /prog`) — safe as long as the program does not mount anything. Do not look
for a test runner; none exists.

## Ground rules for editing C (freestanding)

- **No libc.** Never `#include <...>` (currently zero angle-bracket includes exist in `src/`).
  Flags enforce it: `-nostdlib -ffreestanding -fno-builtin -fno-stack-protector`.
- All files compile together with `-I src/`, so includes are written project-relative:
  `"lib/lib.h"`, `"kernel/syscall.h"`, `"mios.h"`. `src/mios.h` is the umbrella header.
- Use project types/helpers, not stdlib: `src/lib/types.h` (`u8..u64`, `mios_size_t`, `MIOS_NULL`),
  `mios_str*` / `mios_mem*`, console via `cons_printf` / `cons_puts` / `cons_flush` /
  `cons_read_line`. There is no system `printf`, `malloc` or `readline`.
- **Syscall boundary:** the `syscall` instruction lives **only** in `src/kernel/syscall.c`. Add new
  kernel calls there as `mios_*` wrappers (declared in `src/kernel/syscall.h`); never issue
  syscalls inline elsewhere.

## Adding a shell command

- Commands are `command_t` tables grouped per subsystem; each table is registered from
  `shell_init()` in `src/core/shell.c` through `cmd_*_register()`
  (`cmd_shell_register`, `cmd_sys_register`, `cmd_hw_register`, `cmd_fs_register`,
  `cmd_net_register`).
- Add entries to the matching `src/*/cmd_*.c` table. If you add a whole new subsystem, write its
  `cmd_*_register()` and call it from `shell_init()`. Handlers take one `char *args` string.
- **Any order that is not an internal command is run as a program** (like a real shell): the
  dispatcher in `src/core/shell.c` calls `prog_run(cmd, args)` (`src/core/prog.c`), which searches
  `PATH`. It returns the child's exit code and only prints when it is non-zero
  (`MiOS: 'x' termino con codigo N`). So adding an executable program means dropping it in
  `userland/` (→ `/bin`) rather than writing a command table.

## Non-obvious layout & facts

- Entry point is `_start` (naked inline asm) in `src/core/main.c`, linked with `-Wl,-e,_start`;
  it calls `mios_entry()`, which **never returns** (PID 1).
- `src/lib/` = own libc — goes into `libmios.so.6`. Fase 1 had: console, string, memory, convert,
  errno, posix. Fase 2 added: `malloc.c` (heap on `brk`), `format.c` (`mios_vformat` engine),
  `stdio.c` (`FILE`/`stdin`/`stdout`/`stderr`), `scanf.c`, `ctype.c`, `stringstd.c`, `stdlib.c`,
  `dirent.c`, `stat.c`, `time.c`, `process.c`, `misc.c` (`__assert_fail`, `__stack_chk_*`), plus
  `ret.h` (`mios_ret`: `-errno`→`-1`). Fase 3 added: `math.c` (own libm: exact parts via bit
  tricks/x87 — `sqrtsd`, `frndint` — and transcendentals via fdlibm-style reduction+minimax or
  series; `float` variants delegate to `double`, `long double` uses x87 where exact) and `fenv.c`
  (x87 + MXCSR kept in sync). Fase 6b added: `termios.c` (`tcgetattr`/`tcsetattr`/`cfmakeraw`/
  `tcflush`, over the same `SYS_ioctl` that `isatty` already used — the console editor needs raw
  mode). Fase 7 added what binutils asked for: `locale.c` (`setlocale`/`localeconv`, only "C"),
  `resource.c` (`getrusage`/`getrlimit`/`setrlimit`/`getpagesize`/`sysconf`), `wchar.c`
  (`mbstowcs`/`wcstombs`), `wctype.c` (`towlower`/`towupper`), `realpath.c` (`realpath`/
  `canonicalize_file_name`, without which libiberty's `lrealpath` has no implementation at all
  and *returns garbage*; `examples/rptest.c` is its test: 15 cases of paths, symlinks and errors),
  `pathconf.c` (`pathconf`/`fpathconf` + the `_PC_*` names that lie in `<unistd.h>`), plus
  `mktemp`, `strcoll`/`strxfrm`, `utime`, `msync`/`MS_*`, `sigset_t`+`sigprocmask` and the full
  Linux `errno.h` set.
  `src/kernel/` = syscall ABI (also into `libmios.so.6`);
  `src/ld/ldmios.c` = the dynamic loader; `src/core/` = entry, boot, shell, `prog.c` (launching
  programs: `fork`+`execvp`+`waitpid`), `cmd_shell.c` (`cd`/`pwd`), modules; `src/fs/` and
  `src/net/` (netlink) are the other subsystems — these last three go into the `init` executable.
- `userland/` = the **MiOS programs** (not part of `init`): `build.sh` compiles them with `mios-gcc`
  into `initramfs/bin/`, which the shell reaches through `PATH=/bin:/usr/bin`. `hola.c` is a
  greeting (uses `printf`, `uname`, `getpid`); `args.c` prints its arguments and exits with
  `argc` (handy to verify argument passing); `selftest.c` is the self-test (`fork`/`waitpid`,
  `execv`/`execvp`, `pipe`, inherited fds) and reports with `cons_*` so its output also reaches the
  serial port — that is how the process machinery is verified headlessly (`/etc/autorun` = `selftest`).
  `runlog.c` (Fase 5) runs another program with a pipe as its stdout/stderr and mirrors everything
  to the screen **and** `/dev/ttyS0`: it is how a program's output (and a compiler's errors) can be
  read from the serial log without a screen. `edit.c` (Fase 6b) is the console text editor: it puts
  the console in raw mode (`cfmakeraw`) while it runs, paints with ANSI escapes, and has two
  non-interactive modes that `/etc/autorun`/`runlog` can drive headlessly: `--selftest` (the editing
  engine) and `--ttytest` (the termios raw-mode ioctls). Do **not** put the interactive editor in
  `/etc/autorun`: it waits for keys and would hang the boot.
  `initramfs/etc/autorun` is optional: the shell runs
  its lines at boot (one order per line, `#` comments), via `shell_autorun()` in `src/core/shell.c`.
- `third_party/tcc/` = the `tcc` sources (0.9.27, **gitignored**; `tools/fetch-tcc.sh` clones them
  pinned to the tag). `tools/build-tcc.sh` generates the `config.h` that `tcc` normally gets from
  `configure` (`CONFIG_TCCDIR=/lib/tcc`, `CONFIG_TCC_ELFINTERP=/lib/ld-mios.so.1`,
  `CONFIG_TCC_CRTPREFIX=/usr/lib`, `CONFIG_TCC_LIBPATHS`, `CONFIG_TCC_SYSINCLUDEPATHS`,
  `CONFIG_TCC_STATIC` — there is no `dlopen` in MiOS — and `CONFIG_TCCBOOT`), compiles `tcc.c` as a
  **single translation unit** (`ONE_SOURCE=1`, which `#include`s `libtcc.c`) with `mios-gcc`, builds
  `libtcc1.a`, and installs everything into the initramfs. It never needs `make` (the VPS has none).
  It also applies a three-line MiOS patch (`tools/tcc-mios-info.inc`) to a **copy** of `tcc.c` in
  `build/` — `third_party/` stays untouched — so `tcc` always reports the result of a compile
  (`tcc: compilacion correcta -> <file>` or `tcc: la compilacion ha fallado`) instead of staying
  silent on success.
  Inside MiOS the public headers live in `/usr/include` and the crt objects in `/usr/lib`, so a
  program compiled by `tcc` links exactly like one compiled by `mios-gcc`. `examples/tcc-hola.c` is
  shipped as `/usr/src/hola.c` as the ready-made test for `tcc` inside MiOS.
- **Self-hosting in C (Fase 6):** `build-tcc.sh` also copies **tcc's own sources** into the ISO as
  `/usr/src/tcc/` (root `*.c`/`*.h`/`*.def` + `lib/` + `include/`, including the generated
  `config.h`; the shipped `tcc.c` is the patched copy, so a self-compiled `tcc` behaves the same),
  so `tcc -o /tmp/tcc2 /usr/src/tcc/tcc.c` compiles the compiler inside MiOS and the
  result works. `ONE_SOURCE` (tcc's default) means that single file pulls in the rest. `libtcc1.a`
  must contain the full x86_64 set from tcc's `lib/Makefile` (`libtcc1.o`, **`va_list.o`**,
  `alloca86_64.o`, `alloca86_64-bt.o`; no `bcheck.o`, since `CONFIG_TCCBOOT` disables `-b`):
  tcc's x86_64 `stdarg.h` sends `va_start`/`va_arg` to `__va_start`/`__va_arg`, which live in
  `lib/va_list.c` — without it any varargs program fails to link.
- `include/` = the **public** headers installed into the sysroot, with standard names
  (`stdio.h`, `string.h`, `stdlib.h`, `unistd.h`, `fcntl.h`, `errno.h`, `ctype.h`, `dirent.h`,
  `time.h`, `signal.h`, `assert.h`, `limits.h`, `math.h`, `fenv.h`, `setjmp.h`, `termios.h`,
  `stdint.h`, `inttypes.h`, `locale.h`, `strings.h`, `wchar.h`, `wctype.h`, `utime.h`, `memory.h`,
  `features.h` (empty shim, on purpose without `__GLIBC__`), `sys/types.h`, `sys/stat.h`,
  `sys/wait.h`, `sys/mman.h`, `sys/time.h`, `sys/resource.h`, `sys/param.h`, `sys/user.h`,
  `sys/procfs.h`, `mios.h`).
  They are *not* the internal headers: they only declare what
  `libmios.so.6` exports, including the standard aliases added by `src/lib/posix.c` and friends
  (`open`, `read`, `write`, `printf`, `exit`, `mkdir`, `uname`, `strlen`, ...). `mk-sysroot.sh`
  also links `libm.so` and `libc.so` to `libmios.so.6` so `-lm`/`-lc` work (there is no separate
  libm). Every typdef is guarded (`MIOS_*_DEFINED`) so the headers can be included in any order.
  **They must also compile with `tcc`**, not just gcc: variadic prototypes use `va_list` (never
  `__builtin_va_list`) and `math.h` falls back to libmios functions (`__mios_isnan`, `__mios_huge_val`,
  `__mios_nan`, ...) with compilers that have no gcc builtins.
  `src/mios.h` (internal umbrella) and `include/mios.h` (public umbrella) are different files;
  shared `struct mios_*` definitions are guarded with `MIOS_DEF_*` on both sides.
- `src/crt/crt1.c` = `_start` for normal programs (argc/argv/envp → `main` → `exit`), built into
  the sysroot as `crt1.o`. It calls `__mios_set_environ(envp)` (libmios) so `getenv`/`environ`
  work. `init` keeps its own `_start` in `src/core/main.c` because PID 1 never returns.
  `src/crt/` is **not** part of the `init` build.
- `errno` is a **function** (`__errno_location()`, like glibc), not a data symbol — kept that way
  even though the loader now supports `R_X86_64_COPY`.
- The loader implements `R_X86_64_COPY` (data symbols the executable keeps a copy of, e.g.
  `stdout`, `environ`) **and interposition**: globally-defined data in a `.so` that the executable
  copies must resolve to the *executable's* copy, otherwise libmios writes its own variable while
  the program reads another. Local symbols and `R_X86_64_RELATIVE` stay module-local.
- **A `-shared` link must not get `crt1.o`**: `mios-gcc` detects `-shared` and then skips `crt1.o`,
  `-e,_start` and `--dynamic-linker` (a `.so` has no `main` and no entry point) and compiles with
  `-fPIC`. This is what builds GMP/MPFR/MPC (libtool calls `cc -shared ...`).
- **`<stdio.h>` defines `_STDIO_H`** (glibc's include-guard name), besides MiOS's own
  `MIOS_STDIO_H`. Third-party code *recognises* `stdio.h` by that macro rather than by `FILE`
  (GMP does: without it, `gmp.h` warns "doesnt recognise <stdio.h>" and hides every `FILE`
  prototype, so `mpz_inp_str.c` fails to compile). Same family: musl's `__DEFINED_FILE`.
- Kernel is **6.8.0-100-generic**; `.ko` modules ship at
  `initramfs/lib/modules/6.8.0-100-generic/`. The module path is built from `uname()` release at
  runtime — don't hardcode it in code.
- Source comments and all console output strings are **Spanish, ASCII (no accents)**; the README
  uses accents but source strings do not.

## Docs

- `README.md` (features, dependency list, docs links)
- `docs/PLAN-TOOLCHAIN.md` — plan completo del toolchain propio (tcc → GCC → self-hosting)
- `docs/PROGRESO.md` — bitácora de avances (actualizar en cada fase)
- `Guia_MiOS_kernel_initramfs_ISO.md`, `Guia_MiOS_init_c_estatico_initramfs_ISO.md`
