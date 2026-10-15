#ifndef MIOS_H
#define MIOS_H

/*
 * MiOS - cabecera comun.
 *
 * MiOS se ejecuta como PID 1 sobre el kernel Linux.
 * No usa libc: todo se pide al kernel con syscalls x86_64.
 *
 * El codigo esta dividido por subsistemas dentro de src/:
 *   lib/     utilidades basicas (texto, memoria, consola, errores)
 *   kernel/  ABI del kernel y capa de syscalls
 *   core/    arranque, shell y comandos del sistema
 *   fs/      archivos y comandos del sistema de archivos
 *   net/     red (ethernet/wifi) sobre netlink
 */

#define MIOS_VERSION "0.5"

#define MIOS_MAX_PATH 512
#define MIOS_MAX_LINE 512

#include "lib/lib.h"
#include "kernel/syscall.h"
#include "core/core.h"
#include "fs/fs.h"
#include "net/net.h"

#endif /* MIOS_H */
