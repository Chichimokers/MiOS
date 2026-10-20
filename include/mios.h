#ifndef MIOS_H
#define MIOS_H

/*
 * MiOS - cabecera general (la que incluyen los programas de MiOS).
 *
 * Agrupa:
 *   1. Las cabeceras "estandar" que ofrece libmios (stdio/string/unistd/...).
 *   2. La API propia de MiOS: consola (cons_*) y las llamadas al kernel (mios_*).
 *
 * MiOS no usa libc: todo lo de aqui lo resuelve libmios.so.6 en tiempo de
 * ejecucion, a traves de ld-mios.so.1.
 *
 * Compilar con:  ./mios-gcc programa.c -o programa
 */

#include <stddef.h>
#include <stdarg.h>

#include "mios_types.h"
#include "errno.h"
#include "fcntl.h"
#include "unistd.h"
#include "string.h"
#include "strings.h"
#include "stdlib.h"
#include "stdio.h"
#include "ctype.h"
#include "dirent.h"
#include "time.h"
#include "signal.h"
#include "limits.h"
#include "assert.h"
#include "inttypes.h"
#include "locale.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "sys/wait.h"
#include "sys/time.h"
#include "sys/resource.h"
#include "utime.h"

/* ------------------------------------------------------------------ */
/* Estructuras del kernel que hacen falta para las llamadas mios_*     */
/* ------------------------------------------------------------------ */

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

#ifndef MIOS_DEF_STAT
#define MIOS_DEF_STAT
struct mios_stat {
    unsigned long st_dev;
    unsigned long st_ino;
    unsigned long st_nlink;
    unsigned int  st_mode;
    unsigned int  st_uid;
    unsigned int  st_gid;
    unsigned int  __pad0;
    unsigned long st_rdev;
    long          st_size;
    long          st_blksize;
    long          st_blocks;
    unsigned long st_atime;
    unsigned long st_atime_nsec;
    unsigned long st_mtime;
    unsigned long st_mtime_nsec;
    unsigned long st_ctime;
    unsigned long st_ctime_nsec;
    long          __unused[3];
};
#endif

#ifndef MIOS_DEF_TIMEVAL
#define MIOS_DEF_TIMEVAL
struct mios_timeval {
    long tv_sec;
    long tv_usec;
};
#endif

#ifndef MIOS_DEF_TIMESPEC
#define MIOS_DEF_TIMESPEC
struct mios_timespec {
    long tv_sec;
    long tv_nsec;
};
#endif

#ifndef MIOS_DEF_MIOS_UTIMBUF
#define MIOS_DEF_MIOS_UTIMBUF
struct mios_utimbuf {
    long actime;
    long modtime;
};
#endif

struct utsname {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
    char domainname[65];
};

/* ------------------------------------------------------------------ */
/* Consola de MiOS (con historial y espejo por el puerto serie)        */
/* ------------------------------------------------------------------ */

void cons_printf(const char *fmt, ...);
int  cons_vprintf(const char *fmt, va_list ap);
void cons_puts(const char *s);
void cons_putc(char c);
void cons_flush(void);
void cons_log_dump(void);
int  cons_read_line(char *buf, size_t size);

/* ------------------------------------------------------------------ */
/* Llamadas al kernel (capa fina sobre syscall)                        */
/* ------------------------------------------------------------------ */

/* Ficheros y directorios */
long mios_read(int fd, void *buf, size_t count);
long mios_write(int fd, const void *buf, size_t count);
long mios_pread(int fd, void *buf, size_t count, long off);
long mios_pwrite(int fd, const void *buf, size_t count, long off);
int  mios_open(const char *path, int flags, int mode);
int  mios_close(int fd);
long mios_lseek(int fd, long offset, int whence);
long mios_getdents64(int fd, void *buf, size_t count);
int  mios_mkdir(const char *path, int mode);
int  mios_rmdir(const char *path);
int  mios_unlink(const char *path);
int  mios_chdir(const char *path);
long mios_getcwd(char *buf, size_t size);
int  mios_rename(const char *oldpath, const char *newpath);
int  mios_mknod(const char *path, int mode, unsigned long dev);
int  mios_mount(const char *src, const char *target, const char *fstype,
                unsigned long flags, const void *data);

/* Memoria */
void *mios_mmap(void *addr, size_t len, int prot, int flags, int fd, long off);
int   mios_munmap(void *addr, size_t len);
int   mios_mprotect(void *addr, size_t len, int prot);
long  mios_brk(void *addr);

/* Metadatos */
int  mios_stat(const char *path, struct mios_stat *st);
int  mios_fstat(int fd, struct mios_stat *st);
int  mios_lstat(const char *path, struct mios_stat *st);
int  mios_access(const char *path, int mode);
int  mios_chmod(const char *path, int mode);
int  mios_fchmod(int fd, int mode);
int  mios_fsync(int fd);
int  mios_chown(const char *path, int uid, int gid);
int  mios_utime(const char *path, const struct mios_utimbuf *times);
int  mios_link(const char *oldpath, const char *newpath);
int  mios_symlink(const char *target, const char *linkpath);
long mios_readlink(const char *path, char *buf, size_t size);
int  mios_umask(int mask);

/* Procesos y sistema */
int  mios_getpid(void);
int  mios_getppid(void);
int  mios_getuid(void);
int  mios_getgid(void);
int  mios_fork(void);
int  mios_execve(const char *path, char *const argv[], char *const envp[]);
int  mios_wait4(int pid, int *status, int options, void *rusage);
int  mios_kill(int pid, int sig);
int  mios_setsid(void);
int  mios_setpgid(int pid, int pgid);
int  mios_getpgid(int pid);
int  mios_sigaction(int sig, const void *act, void *oldact);
int  mios_sigprocmask(int how, const void *set, void *oldset);
int  mios_ioctl(int fd, unsigned long request, void *arg);
int  mios_fcntl(int fd, int cmd, long arg);
int  mios_dup(int fd);
int  mios_dup2(int oldfd, int newfd);
int  mios_pipe(int *fds);
int  mios_uname(struct mios_utsname *u);
void mios_sleep(long seconds);
int  mios_sleep_ns(long seconds, long nanoseconds);
void mios_exit(int code) __attribute__((noreturn));
int  mios_reboot(int magic1, int magic2, int cmd);
int  mios_error(long ret);

/* Tiempo */
long mios_time(long *tloc);
int  mios_gettimeofday(struct mios_timeval *tv, void *tz);
int  mios_clock_gettime(int clockid, struct mios_timespec *ts);
int  mios_clock_getres(int clockid, struct mios_timespec *ts);

/* Limites y consumo de recursos */
int  mios_getrusage(int who, void *usage);
int  mios_getrlimit(int resource, void *rlim);
int  mios_setrlimit(int resource, const void *rlim);

/* Modulos del kernel */
int  mios_finit_module(int fd, const char *params, int flags);

/* Red (sockets crudos) */
int  mios_socket(int domain, int type, int protocol);
int  mios_bind(int fd, const void *addr, unsigned int addrlen);
long mios_sendto(int fd, const void *buf, size_t len, int flags,
                 const void *addr, unsigned int addrlen);
long mios_recvfrom(int fd, void *buf, size_t len, int flags,
                   void *addr, unsigned int *addrlen);

/* Primitiva cruda (uso interno; normalmente no hace falta) */
long mios_syscall0(long n);
long mios_syscall1(long n, long a1);
long mios_syscall2(long n, long a1, long a2);
long mios_syscall3(long n, long a1, long a2, long a3);
long mios_syscall4(long n, long a1, long a2, long a3, long a4);
long mios_syscall5(long n, long a1, long a2, long a3, long a4, long a5);
long mios_syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6);

/* ------------------------------------------------------------------ */
/* Nombres estandar que no viven en una cabecera "de sistema"          */
/* ------------------------------------------------------------------ */

int uname(struct utsname *u);
int rmdir(const char *path);
int rename(const char *oldpath, const char *newpath);
int mknod(const char *path, int mode, unsigned long dev);
int mount(const char *src, const char *target, const char *fstype,
          unsigned long flags, const void *data);
void reboot(int magic1, int magic2, int cmd);

/* Constantes utiles */
#define MIOS_DT_CHR 2
#define MIOS_DT_DIR 4
#define MIOS_DT_BLK 6
#define MIOS_DT_REG 8
#define MIOS_DT_LNK 10

#define MIOS_S_IFMT  0170000
#define MIOS_S_IFDIR 0040000
#define MIOS_S_IFCHR 0020000
#define MIOS_S_IFBLK 0060000

#endif /* MIOS_H */
