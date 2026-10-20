#ifndef MIOS_KERNEL_H
#define MIOS_KERNEL_H

#include "lib/types.h"

/*
 * MiOS - ABI del kernel Linux (x86_64).
 * Estructuras y constantes que el kernel espera en sus syscalls.
 */

#ifndef MIOS_DEF_UTSNAME
#define MIOS_DEF_UTSNAME
struct mios_utsname {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
    char domainname[65];
};
#endif

#ifndef MIOS_DEF_DIRENT64
#define MIOS_DEF_DIRENT64
struct mios_dirent64 {
    u64 d_ino;
    s64 d_off;
    u16 d_reclen;
    u8  d_type;
    char d_name[];
};
#endif

#ifndef MIOS_DEF_TIMESPEC
#define MIOS_DEF_TIMESPEC
struct mios_timespec {
    s64 tv_sec;
    s64 tv_nsec;
};
#endif

#ifndef MIOS_DEF_TIMEVAL
#define MIOS_DEF_TIMEVAL
struct mios_timeval {
    s64 tv_sec;
    s64 tv_usec;
};
#endif

/* struct rlimit de Linux x86_64 (getrlimit/setrlimit). */
#ifndef MIOS_DEF_MIOS_RLIMIT
#define MIOS_DEF_MIOS_RLIMIT
struct mios_rlimit {
    u64 rlim_cur;
    u64 rlim_max;
};
#endif

/* struct rusage de Linux x86_64 (getrusage). */
#ifndef MIOS_DEF_MIOS_RUSAGE
#define MIOS_DEF_MIOS_RUSAGE
struct mios_rusage {
    struct mios_timeval ru_utime;
    struct mios_timeval ru_stime;
    s64 ru_maxrss;
    s64 ru_ixrss;
    s64 ru_idrss;
    s64 ru_isrss;
    s64 ru_minflt;
    s64 ru_majflt;
    s64 ru_nswap;
    s64 ru_inblock;
    s64 ru_oublock;
    s64 ru_msgsnd;
    s64 ru_msgrcv;
    s64 ru_nsignals;
    s64 ru_nvcsw;
    s64 ru_nivcsw;
};
#endif

/* struct utimbuf de Linux x86_64 (utime). */
#ifndef MIOS_DEF_MIOS_UTIMBUF
#define MIOS_DEF_MIOS_UTIMBUF
struct mios_utimbuf {
    s64 actime;
    s64 modtime;
};
#endif

/* struct stat de x86_64 (el que espera newfstatat). */
#ifndef MIOS_DEF_STAT
#define MIOS_DEF_STAT
struct mios_stat {
    u64 st_dev;
    u64 st_ino;
    u64 st_nlink;
    u32 st_mode;
    u32 st_uid;
    u32 st_gid;
    u32 __pad0;
    u64 st_rdev;
    s64 st_size;
    s64 st_blksize;
    s64 st_blocks;
    u64 st_atime;
    u64 st_atime_nsec;
    u64 st_mtime;
    u64 st_mtime_nsec;
    u64 st_ctime;
    u64 st_ctime_nsec;
    s64 __unused[3];
};
#endif

#define MIOS_DT_CHR 2
#define MIOS_DT_DIR 4
#define MIOS_DT_BLK 6
#define MIOS_DT_REG 8
#define MIOS_DT_LNK 10

/* Flags de open() */
#define MIOS_O_RDONLY 0
#define MIOS_O_WRONLY 1
#define MIOS_O_RDWR   2
#define MIOS_O_ACCMODE 3
#define MIOS_O_CREAT  0100
#define MIOS_O_EXCL   0200
#define MIOS_O_NOCTTY 0400
#define MIOS_O_TRUNC  01000
#define MIOS_O_APPEND 02000
#define MIOS_O_NONBLOCK 04000
#define MIOS_O_DIRECTORY 0200000
#define MIOS_O_CLOEXEC 02000000

/* Modos de archivo */
#define MIOS_S_IFMT  0170000
#define MIOS_S_IFDIR 0040000
#define MIOS_S_IFCHR 0020000
#define MIOS_S_IFBLK 0060000
#define MIOS_S_IFREG 0100000
#define MIOS_S_IFLNK 0120000
#define MIOS_S_IFIFO 0010000
#define MIOS_S_IFSOCK 0140000

/* lseek */
#define MIOS_SEEK_SET 0
#define MIOS_SEEK_CUR 1
#define MIOS_SEEK_END 2

/* mmap */
#define MIOS_PROT_READ  1
#define MIOS_PROT_WRITE 2
#define MIOS_PROT_EXEC  4
#define MIOS_MAP_PRIVATE   0x02
#define MIOS_MAP_SHARED    0x01
#define MIOS_MAP_ANONYMOUS 0x20
#define MIOS_MAP_FAILED    ((void *)-1)

/* fstatat / access / fcntl / relojes */
#define MIOS_AT_SYMLINK_NOFOLLOW 0x100
#define MIOS_AT_EMPTY_PATH       0x1000
#define MIOS_F_DUPFD 0
#define MIOS_F_GETFD 1
#define MIOS_F_SETFD 2
#define MIOS_F_GETFL 3
#define MIOS_F_SETFL 4
#define MIOS_CLOCK_REALTIME  0
#define MIOS_CLOCK_MONOTONIC 1
#define MIOS_WNOHANG 1

/* ioctl */
#define MIOS_TIOCGWINSZ 0x5413
#define MIOS_TIOCGPGRP  0x540F
#define MIOS_TIOCSPGRP  0x5410

/* Senales (numeros de Linux) */
#define MIOS_SIGHUP  1
#define MIOS_SIGINT  2
#define MIOS_SIGQUIT 3
#define MIOS_SIGILL  4
#define MIOS_SIGTRAP 5
#define MIOS_SIGABRT 6
#define MIOS_SIGBUS  7
#define MIOS_SIGFPE  8
#define MIOS_SIGKILL 9
#define MIOS_SIGUSR1 10
#define MIOS_SIGSEGV 11
#define MIOS_SIGUSR2 12
#define MIOS_SIGPIPE 13
#define MIOS_SIGALRM 14
#define MIOS_SIGTERM 15
#define MIOS_SIGCHLD 17
#define MIOS_SIGCONT 18
#define MIOS_SIGSTOP 19
#define MIOS_SIGTSTP 20

#endif /* MIOS_KERNEL_H */
