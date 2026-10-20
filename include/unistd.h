#ifndef MIOS_UNISTD_H
#define MIOS_UNISTD_H

/*
 * MiOS - unistd.h (cabecera publica).
 */

#include <stddef.h>
#include "mios_types.h"

#ifndef MIOS_SSIZE_T_DEFINED
#define MIOS_SSIZE_T_DEFINED
typedef long ssize_t;
#endif

#ifndef MIOS_OFF_T_DEFINED
#define MIOS_OFF_T_DEFINED
typedef long off_t;
#endif

#ifndef MIOS_PID_T_DEFINED
#define MIOS_PID_T_DEFINED
typedef int pid_t;
#endif

/* Entrada/salida */
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
ssize_t pread(int fd, void *buf, size_t count, off_t offset);
ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset);
int     close(int fd);
off_t   lseek(int fd, off_t offset, int whence);
int     dup(int fd);
int     dup2(int oldfd, int newfd);
int     pipe(int *fds);
int     isatty(int fd);

/* Ficheros */
int     unlink(const char *path);
int     link(const char *oldpath, const char *newpath);
int     symlink(const char *target, const char *linkpath);
ssize_t readlink(const char *path, char *buf, size_t size);
int     rmdir(const char *path);
int     chdir(const char *path);
char   *getcwd(char *buf, size_t size);
int     access(const char *path, int mode);
int     truncate(const char *path, off_t length);
int     ftruncate(int fd, off_t length);
int     fsync(int fd);

#define F_OK 0
#define X_OK 1
#define W_OK 2
#define R_OK 4

/* Procesos */
pid_t   fork(void);
int     execve(const char *path, char *const argv[], char *const envp[]);
int     execv(const char *path, char *const argv[]);
int     execvp(const char *file, char *const argv[]);
pid_t   getpid(void);
pid_t   getppid(void);
int     getuid(void);
int     getgid(void);
pid_t   setsid(void);
pid_t   setpgid(pid_t pid, pid_t pgid);
pid_t   getpgid(pid_t pid);
int     kill(pid_t pid, int sig);
unsigned int sleep(unsigned int seconds);
int     usleep(unsigned int usec);
int     chown(const char *path, int uid, int gid);
void    _exit(int status) __attribute__((noreturn));

/* Consultas del sistema (Fase 7: las piden binutils/gcc) */
int     getpagesize(void);

#define _SC_ARG_MAX          0
#define _SC_CHILD_MAX        1
#define _SC_CLK_TCK          2
#define _SC_NGROUPS_MAX      3
#define _SC_OPEN_MAX         4
#define _SC_STREAM_MAX       5
#define _SC_TZNAME_MAX       6
#define _SC_JOB_CONTROL      7
#define _SC_SAVED_IDS        8
#define _SC_VERSION          29
#define _SC_PAGESIZE         30
#define _SC_PAGE_SIZE        _SC_PAGESIZE
#define _SC_NPROCESSORS_CONF 83
#define _SC_NPROCESSORS_ONLN 84
#define _SC_PHYS_PAGES       85
#define _SC_AVPHYS_PAGES     86
#define _SC_HOST_NAME_MAX    180
#define _SC_LINE_MAX         203

long    sysconf(int name);

/* Limites por ruta (Fase 7: libiberty's lrealpath los consulta para saber el
   tamano maximo de una ruta).  Los valores son los de Linux/POSIX. */
#define _PC_LINK_MAX          0
#define _PC_MAX_CANON         1
#define _PC_MAX_INPUT         2
#define _PC_NAME_MAX          3
#define _PC_PATH_MAX          4
#define _PC_PIPE_BUF          5
#define _PC_CHOWN_RESTRICTED  6
#define _PC_NO_TRUNC          7
#define _PC_VDISABLE          8
#define _PC_SYNC_IO           9
#define _PC_ASYNC_IO          10
#define _PC_PRIO_IO           11
#define _PC_SOCK_MAXBUF       12
#define _PC_FILESIZEBITS      13
#define _PC_REC_INCR_XFER_SIZE 14
#define _PC_REC_MAX_XFER_SIZE 15
#define _PC_REC_MIN_XFER_SIZE 16
#define _PC_REC_XFER_ALIGN    17
#define _PC_ALLOC_SIZE_MIN    18
#define _PC_SYMLINK_MAX       19
#define _PC_2_SYMLINKS        20

long    pathconf(const char *path, int name);
long    fpathconf(int fd, int name);

#endif /* MIOS_UNISTD_H */
