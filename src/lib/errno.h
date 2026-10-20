#ifndef MIOS_LIB_ERRNO_H
#define MIOS_LIB_ERRNO_H

/*
 * MiOS - codigos de error (coinciden con Linux) y errno.
 *
 * errno se obtiene con __errno_location() y no como variable global: asi el
 * ejecutable no necesita una relocalizacion R_X86_64_COPY para leerlo.
 */

#define MIOS_EPERM   1
#define MIOS_ENOENT  2
#define MIOS_ESRCH   3
#define MIOS_EINTR   4
#define MIOS_EIO     5
#define MIOS_ENXIO   6
#define MIOS_E2BIG   7
#define MIOS_ENOEXEC 8
#define MIOS_EBADF   9
#define MIOS_ECHILD  10
#define MIOS_EAGAIN  11
#define MIOS_ENOMEM  12
#define MIOS_EACCES  13
#define MIOS_EFAULT  14
#define MIOS_EBUSY   16
#define MIOS_EEXIST  17
#define MIOS_EXDEV   18
#define MIOS_ENODEV  19
#define MIOS_ENOTDIR 20
#define MIOS_EISDIR  21
#define MIOS_EINVAL  22
#define MIOS_ENFILE  23
#define MIOS_EMFILE  24
#define MIOS_ENOTTY  25
#define MIOS_ESPIPE  29
#define MIOS_EROFS   30
#define MIOS_EPIPE   32
#define MIOS_EDOM    33
#define MIOS_ERANGE  34
#define MIOS_ENOSPC       28
#define MIOS_ENAMETOOLONG 36
#define MIOS_ENOSYS       38
#define MIOS_ENOTEMPTY    39
#define MIOS_EILSEQ       84
#define MIOS_ENOTSUP      95
#define MIOS_EOVERFLOW    75

int *__errno_location(void);

const char *mios_strerror(int err);

#endif /* MIOS_LIB_ERRNO_H */
