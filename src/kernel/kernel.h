#ifndef MIOS_KERNEL_H
#define MIOS_KERNEL_H

#include "lib/types.h"

/*
 * MiOS - ABI del kernel Linux (x86_64).
 * Estructuras y constantes que el kernel espera en sus syscalls.
 */

struct mios_utsname {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
    char domainname[65];
};

struct mios_dirent64 {
    u64 d_ino;
    s64 d_off;
    u16 d_reclen;
    u8  d_type;
    char d_name[];
};

struct mios_timespec {
    s64 tv_sec;
    s64 tv_nsec;
};

#define MIOS_DT_CHR 2
#define MIOS_DT_DIR 4
#define MIOS_DT_BLK 6
#define MIOS_DT_REG 8
#define MIOS_DT_LNK 10

/* Flags de open() */
#define MIOS_O_RDONLY 0
#define MIOS_O_WRONLY 1
#define MIOS_O_RDWR   2
#define MIOS_O_CREAT  0100
#define MIOS_O_TRUNC  01000
#define MIOS_O_APPEND 02000
#define MIOS_O_NONBLOCK 04000

/* Modos de archivo */
#define MIOS_S_IFMT  0170000
#define MIOS_S_IFDIR 0040000
#define MIOS_S_IFCHR 0020000
#define MIOS_S_IFBLK 0060000
#define MIOS_S_IFREG 0100000
#define MIOS_S_IFLNK 0120000

#endif /* MIOS_KERNEL_H */
