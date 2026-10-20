#include "lib/lib.h"
#include "kernel/syscall.h"
#include "../../include/mios.h"
#include "../../include/unistd.h"
#include "../../include/limits.h"
#include "../../include/errno.h"

/*
 * MiOS - limites por ruta: pathconf() y fpathconf() (unistd.h).
 *
 * POSIX deja que la funcion devuelva -1 cuando el limite no esta definido (sin
 * tocar errno).  Binutils lo necesita: libiberty's lrealpath() pregunta por
 * _PC_PATH_MAX para saber de que tamano tiene que ser el bufer que le pasa a
 * realpath(), y bfd/gas lo consultan en algun sitio mas.
 *
 * MiOS no impone limites propios (todo va sobre el sistema de ficheros del
 * kernel Linux), asi que se responden los valores habituales de Linux.
 */

static long mios_conf_value(int name) {
    switch (name) {
    case _PC_LINK_MAX:
        return 127;
    case _PC_MAX_CANON:
    case _PC_MAX_INPUT:
    case _PC_NAME_MAX:
        return 255;
    case _PC_PATH_MAX:
        return (long)PATH_MAX;
    case _PC_PIPE_BUF:
    case _PC_ALLOC_SIZE_MIN:
        return 4096;
    case _PC_CHOWN_RESTRICTED:
    case _PC_NO_TRUNC:
    case _PC_SYNC_IO:
    case _PC_2_SYMLINKS:
        return 1;
    case _PC_VDISABLE:
        return 0;
    case _PC_FILESIZEBITS:
        return 64;
    case _PC_REC_XFER_ALIGN:
        return 8;
    case _PC_SYMLINK_MAX:
        return 255;
    default:
        errno = EINVAL;
        return -1;
    }
}

long pathconf(const char *path, int name) {
    if (path == MIOS_NULL) {
        errno = EINVAL;
        return -1;
    }
    return mios_conf_value(name);
}

long fpathconf(int fd, int name) {
    (void)fd;
    return mios_conf_value(name);
}
