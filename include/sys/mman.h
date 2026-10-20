#ifndef MIOS_SYS_MMAN_H
#define MIOS_SYS_MMAN_H

/*
 * MiOS - sys/mman.h (cabecera publica).
 *
 * mmap/munmap/mprotect son llamadas al kernel (las mismas de Linux x86_64),
 * asi que los valores de PROT_* y MAP_* son los de Linux.
 */

#include "sys/types.h"

#define PROT_NONE      0x0
#define PROT_READ      0x1
#define PROT_WRITE     0x2
#define PROT_EXEC      0x4
#define PROT_GROWSDOWN 0x01000000
#define PROT_GROWSUP   0x02000000

#define MAP_SHARED     0x01
#define MAP_PRIVATE    0x02
#define MAP_FIXED      0x10
#define MAP_ANONYMOUS  0x20
#define MAP_ANON       MAP_ANONYMOUS
#define MAP_GROWSDOWN  0x00100

#define MAP_FAILED     ((void *)-1)

/* Flags de msync (los de Linux). */
#define MS_ASYNC       1
#define MS_INVALIDATE  2
#define MS_SYNC        4

void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off);
int   munmap(void *addr, size_t len);
int   mprotect(void *addr, size_t len, int prot);
int   msync(void *addr, size_t len, int flags);

#endif /* MIOS_SYS_MMAN_H */
