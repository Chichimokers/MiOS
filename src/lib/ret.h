#ifndef MIOS_LIB_RET_H
#define MIOS_LIB_RET_H

#include "lib/lib.h"
#include "kernel/syscall.h"

/*
 * MiOS - traduccion del convenio de errores.
 *
 * La capa de syscalls devuelve -errno (ya deja el codigo en errno); las
 * funciones "estandar" de C/POSIX devuelven -1.
 */

static inline long mios_ret(long r) {
    return (mios_error(r) != 0) ? -1 : r;
}

#endif /* MIOS_LIB_RET_H */
