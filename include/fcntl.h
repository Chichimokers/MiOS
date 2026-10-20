#ifndef MIOS_FCNTL_H
#define MIOS_FCNTL_H

/*
 * MiOS - fcntl.h (cabecera publica).
 * Los valores coinciden con el ABI de Linux x86_64.
 */

#include "mios_types.h"

#ifndef MIOS_MODE_T_DEFINED
#define MIOS_MODE_T_DEFINED
typedef unsigned int mode_t;
#endif

#define O_RDONLY    0
#define O_WRONLY    1
#define O_RDWR      2
#define O_ACCMODE   3
#define O_CREAT     0100
#define O_EXCL      0200
#define O_NOCTTY    0400
#define O_TRUNC     01000
#define O_APPEND    02000
#define O_NONBLOCK  04000
#define O_DIRECTORY 0200000
#define O_CLOEXEC   02000000

/* fcntl() */
#define F_DUPFD  0
#define F_GETFD  1
#define F_SETFD  2
#define F_GETFL  3
#define F_SETFL  4
#define FD_CLOEXEC 1

int open(const char *path, int flags, ...);
int creat(const char *path, mode_t mode);
int fcntl(int fd, int cmd, ...);
int ioctl(int fd, unsigned long request, ...);

#endif /* MIOS_FCNTL_H */
