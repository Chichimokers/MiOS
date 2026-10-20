#include "mios.h"

/*
 * MiOS - capa de llamadas al sistema (x86_64).
 *
 * Estas son las unicas funciones que hablan con el kernel.
 * Los numeros de syscall corresponden a Linux x86_64.
 *
 * Convenio: devuelven el valor tal cual lo da el kernel, es decir -errno si
 * hay error (para eso esta mios_error()). Ademas dejan el codigo en errno,
 * como espera el codigo C normal.
 */

 
#define SYS_read        0
#define SYS_write       1
#define SYS_open        2
#define SYS_close       3
#define SYS_fstat       5
#define SYS_lseek       8
#define SYS_mmap        9
#define SYS_mprotect    10
#define SYS_munmap      11
#define SYS_brk         12
#define SYS_ioctl       16
#define SYS_pread64     17
#define SYS_pwrite64    18
#define SYS_access      21
#define SYS_pipe        22
#define SYS_msync       26
#define SYS_dup         32
#define SYS_dup2        33
#define SYS_nanosleep   35
#define SYS_alarm       37
#define SYS_getpid      39
#define SYS_socket      41
#define SYS_sendto      44
#define SYS_recvfrom    45
#define SYS_bind        49
#define SYS_clone       56
#define SYS_fork        57
#define SYS_execve      59
#define SYS_exit        60
#define SYS_wait4       61
#define SYS_kill        62
#define SYS_uname       63
#define SYS_fcntl       72
#define SYS_ftruncate   77
#define SYS_getcwd      79
#define SYS_chdir       80
#define SYS_rename      82
#define SYS_mkdir       83
#define SYS_rmdir       84
#define SYS_link        86
#define SYS_unlink      87
#define SYS_symlink     88
#define SYS_readlink    89
#define SYS_chmod       90
#define SYS_chown       92
#define SYS_umask       95
#define SYS_getrlimit   97
#define SYS_getrusage   98
#define SYS_utime       132
#define SYS_gettimeofday 96
#define SYS_getuid      102
#define SYS_getgid      104
#define SYS_getppid     110
#define SYS_setpgid     109
#define SYS_getpgid     121
#define SYS_setsid      112
#define SYS_rt_sigaction 13
#define SYS_rt_sigprocmask 14
#define SYS_fsync       74
#define SYS_fchmod      91
#define SYS_mount       165
#define SYS_reboot      169
#define SYS_setrlimit   160
#define SYS_time        201
#define SYS_getdents64  217
#define SYS_clock_gettime 228
#define SYS_clock_getres 229
#define SYS_exit_group  231
#define SYS_truncate    76
#define SYS_newfstatat  262
#define SYS_mknodat     259
#define SYS_fchmodat    268
#define SYS_finit_module 313
#define SYS_getrandom   318

#define MIOS_AT_FDCWD (-100)

/* Deja el codigo de error en errno y devuelve el valor original. */
static long sys_ret(long ret) {
    int err = mios_error(ret);
    if (err != 0) {
        *__errno_location() = err;
    }
    return ret;
}

long mios_syscall0(long n) {
    long ret;
    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "a"(n)
                     : "rcx", "r11", "memory");
    return ret;
}

long mios_syscall1(long n, long a1) {
    long ret;
    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "a"(n), "D"(a1)
                     : "rcx", "r11", "memory");
    return ret;
}

long mios_syscall2(long n, long a1, long a2) {
    long ret;
    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "a"(n), "D"(a1), "S"(a2)
                     : "rcx", "r11", "memory");
    return ret;
}

long mios_syscall3(long n, long a1, long a2, long a3) {
    long ret;
    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "a"(n), "D"(a1), "S"(a2), "d"(a3)
                     : "rcx", "r11", "memory");
    return ret;
}

long mios_syscall4(long n, long a1, long a2, long a3, long a4) {
    long ret;
    register long r10 __asm__("r10") = a4;
    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10)
                     : "rcx", "r11", "memory");
    return ret;
}

long mios_syscall5(long n, long a1, long a2, long a3, long a4, long a5) {
    long ret;
    register long r10 __asm__("r10") = a4;
    register long r8  __asm__("r8")  = a5;
    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8)
                     : "rcx", "r11", "memory");
    return ret;
}

long mios_syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6) {
    long ret;
    register long r10 __asm__("r10") = a4;
    register long r8  __asm__("r8")  = a5;
    register long r9  __asm__("r9")  = a6;
    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "a"(n), "D"(a1), "S"(a2), "d"(a3),
                       "r"(r10), "r"(r8), "r"(r9)
                     : "rcx", "r11", "memory");
    return ret;
}

int mios_error(long ret) {
    if (ret < 0 && ret >= -4095) {
        return (int)(-ret);
    }
    return 0;
}

long mios_read(int fd, void *buf, mios_size_t count) {
    return sys_ret(mios_syscall3(SYS_read, fd, (long)buf, (long)count));
}

long mios_write(int fd, const void *buf, mios_size_t count) {
    return sys_ret(mios_syscall3(SYS_write, fd, (long)buf, (long)count));
}

long mios_pread(int fd, void *buf, mios_size_t count, long off) {
    return sys_ret(mios_syscall4(SYS_pread64, fd, (long)buf, (long)count, off));
}

long mios_pwrite(int fd, const void *buf, mios_size_t count, long off) {
    return sys_ret(mios_syscall4(SYS_pwrite64, fd, (long)buf, (long)count, off));
}

int mios_open(const char *path, int flags, int mode) {
    return (int)sys_ret(mios_syscall3(SYS_open, (long)path, flags, mode));
}

int mios_close(int fd) {
    return (int)sys_ret(mios_syscall1(SYS_close, fd));
}

long mios_lseek(int fd, long offset, int whence) {
    return sys_ret(mios_syscall3(SYS_lseek, fd, offset, whence));
}

long mios_getdents64(int fd, void *buf, mios_size_t count) {
    return sys_ret(mios_syscall3(SYS_getdents64, fd, (long)buf, (long)count));
}

int mios_mount(const char *src, const char *target, const char *fstype,
               unsigned long flags, const void *data) {
    return (int)sys_ret(mios_syscall5(SYS_mount, (long)src, (long)target,
                                      (long)fstype, (long)flags, (long)data));
}

int mios_mkdir(const char *path, int mode) {
    return (int)sys_ret(mios_syscall2(SYS_mkdir, (long)path, mode));
}

int mios_rmdir(const char *path) {
    return (int)sys_ret(mios_syscall1(SYS_rmdir, (long)path));
}

int mios_chdir(const char *path) {
    return (int)sys_ret(mios_syscall1(SYS_chdir, (long)path));
}

long mios_getcwd(char *buf, mios_size_t size) {
    return sys_ret(mios_syscall2(SYS_getcwd, (long)buf, (long)size));
}

int mios_rename(const char *oldpath, const char *newpath) {
    return (int)sys_ret(mios_syscall2(SYS_rename, (long)oldpath, (long)newpath));
}

int mios_unlink(const char *path) {
    return (int)sys_ret(mios_syscall1(SYS_unlink, (long)path));
}

int mios_mknod(const char *path, int mode, unsigned long dev) {
    return (int)sys_ret(mios_syscall4(SYS_mknodat, MIOS_AT_FDCWD, (long)path,
                                      mode, (long)dev));
}

int mios_uname(struct mios_utsname *u) {
    return (int)sys_ret(mios_syscall1(SYS_uname, (long)u));
}

int mios_getpid(void) {
    return (int)sys_ret(mios_syscall0(SYS_getpid));
}

int mios_getppid(void) {
    return (int)sys_ret(mios_syscall0(SYS_getppid));
}

int mios_getuid(void) {
    return (int)sys_ret(mios_syscall0(SYS_getuid));
}

int mios_getgid(void) {
    return (int)sys_ret(mios_syscall0(SYS_getgid));
}

int mios_reboot(int magic1, int magic2, int cmd) {
    return (int)sys_ret(mios_syscall4(SYS_reboot, magic1, magic2, cmd, 0));
}

void mios_exit(int code) {
    mios_syscall1(SYS_exit_group, code);
    for (;;) {
        /* El PID 1 nunca debe llegar aqui. */
    }
}

void mios_sleep(long seconds) {
    struct mios_timespec ts;
    ts.tv_sec = seconds;
    ts.tv_nsec = 0;
    sys_ret(mios_syscall2(SYS_nanosleep, (long)&ts, 0));
}

int mios_sleep_ns(long seconds, long nanoseconds) {
    struct mios_timespec ts;
    ts.tv_sec = seconds;
    ts.tv_nsec = nanoseconds;
    return (int)sys_ret(mios_syscall2(SYS_nanosleep, (long)&ts, 0));
}

/* ------------------------------------------------------------------ */
/* Memoria                                                             */
/* ------------------------------------------------------------------ */

void *mios_mmap(void *addr, mios_size_t len, int prot, int flags, int fd, long off) {
    long r = sys_ret(mios_syscall6(SYS_mmap, (long)addr, (long)len, prot,
                                   flags, fd, off));
    if (mios_error(r)) {
        return MIOS_MAP_FAILED;
    }
    return (void *)r;
}

int mios_munmap(void *addr, mios_size_t len) {
    return (int)sys_ret(mios_syscall2(SYS_munmap, (long)addr, (long)len));
}

int mios_mprotect(void *addr, mios_size_t len, int prot) {
    return (int)sys_ret(mios_syscall3(SYS_mprotect, (long)addr, (long)len, prot));
}

int mios_msync(void *addr, mios_size_t len, int flags) {
    return (int)sys_ret(mios_syscall3(SYS_msync, (long)addr, (long)len, flags));
}

long mios_brk(void *addr) {
    return sys_ret(mios_syscall1(SYS_brk, (long)addr));
}

/* ------------------------------------------------------------------ */
/* Metadatos de ficheros                                               */
/* ------------------------------------------------------------------ */

int mios_fstat(int fd, struct mios_stat *st) {
    return (int)sys_ret(mios_syscall2(SYS_fstat, fd, (long)st));
}

int mios_stat(const char *path, struct mios_stat *st) {
    return (int)sys_ret(mios_syscall4(SYS_newfstatat, MIOS_AT_FDCWD, (long)path,
                                      (long)st, 0));
}

int mios_lstat(const char *path, struct mios_stat *st) {
    return (int)sys_ret(mios_syscall4(SYS_newfstatat, MIOS_AT_FDCWD, (long)path,
                                      (long)st, MIOS_AT_SYMLINK_NOFOLLOW));
}

int mios_access(const char *path, int mode) {
    return (int)sys_ret(mios_syscall2(SYS_access, (long)path, mode));
}

int mios_chmod(const char *path, int mode) {
    return (int)sys_ret(mios_syscall3(SYS_fchmodat, MIOS_AT_FDCWD, (long)path,
                                      mode));
}

int mios_chown(const char *path, int uid, int gid) {
    return (int)sys_ret(mios_syscall3(SYS_chown, (long)path, uid, gid));
}

/* times == MIOS_NULL significa "ahora" (lo decide el kernel). */
int mios_utime(const char *path, const struct mios_utimbuf *times) {
    return (int)sys_ret(mios_syscall2(SYS_utime, (long)path, (long)times));
}

int mios_link(const char *oldpath, const char *newpath) {
    return (int)sys_ret(mios_syscall2(SYS_link, (long)oldpath, (long)newpath));
}

int mios_symlink(const char *target, const char *linkpath) {
    return (int)sys_ret(mios_syscall2(SYS_symlink, (long)target, (long)linkpath));
}

long mios_readlink(const char *path, char *buf, mios_size_t size) {
    return sys_ret(mios_syscall3(SYS_readlink, (long)path, (long)buf, (long)size));
}

int mios_umask(int mask) {
    return (int)sys_ret(mios_syscall1(SYS_umask, mask));
}

int mios_ftruncate(int fd, long length) {
    return (int)sys_ret(mios_syscall2(SYS_ftruncate, fd, length));
}

int mios_truncate(const char *path, long length) {
    return (int)sys_ret(mios_syscall2(SYS_truncate, (long)path, length));
}

/* ------------------------------------------------------------------ */
/* Descriptores                                                        */
/* ------------------------------------------------------------------ */

int mios_dup(int fd) {
    return (int)sys_ret(mios_syscall1(SYS_dup, fd));
}

int mios_dup2(int oldfd, int newfd) {
    return (int)sys_ret(mios_syscall2(SYS_dup2, oldfd, newfd));
}

int mios_pipe(int *fds) {
    return (int)sys_ret(mios_syscall1(SYS_pipe, (long)fds));
}

int mios_fcntl(int fd, int cmd, long arg) {
    return (int)sys_ret(mios_syscall3(SYS_fcntl, fd, cmd, arg));
}

int mios_ioctl(int fd, unsigned long request, void *arg) {
    return (int)sys_ret(mios_syscall3(SYS_ioctl, fd, (long)request, (long)arg));
}

/* ------------------------------------------------------------------ */
/* Procesos                                                            */
/* ------------------------------------------------------------------ */

int mios_fork(void) {
    return (int)sys_ret(mios_syscall0(SYS_fork));
}

int mios_execve(const char *path, char *const argv[], char *const envp[]) {
    return (int)sys_ret(mios_syscall3(SYS_execve, (long)path, (long)argv,
                                      (long)envp));
}

int mios_wait4(int pid, int *status, int options, void *rusage) {
    return (int)sys_ret(mios_syscall4(SYS_wait4, pid, (long)status, options,
                                      (long)rusage));
}

int mios_kill(int pid, int sig) {
    return (int)sys_ret(mios_syscall2(SYS_kill, pid, sig));
}

int mios_setsid(void) {
    return (int)sys_ret(mios_syscall0(SYS_setsid));
}

int mios_setpgid(int pid, int pgid) {
    return (int)sys_ret(mios_syscall2(SYS_setpgid, pid, pgid));
}

int mios_getpgid(int pid) {
    return (int)sys_ret(mios_syscall1(SYS_getpgid, pid));
}

int mios_fchmod(int fd, int mode) {
    return (int)sys_ret(mios_syscall2(SYS_fchmod, fd, mode));
}

int mios_fsync(int fd) {
    return (int)sys_ret(mios_syscall1(SYS_fsync, fd));
}

int mios_sigprocmask(int how, const void *set, void *oldset) {
    return (int)sys_ret(mios_syscall4(SYS_rt_sigprocmask, how, (long)set,
                                      (long)oldset, 8));
}

int mios_sigaction(int sig, const void *act, void *oldact) {
    /* rt_sigaction: struct sigaction de x86_64 = 32 bytes (handler, flags,
     * restorer, mask). */
    return (int)sys_ret(mios_syscall4(SYS_rt_sigaction, sig, (long)act,
                                      (long)oldact, 8));
}

/* ------------------------------------------------------------------ */
/* Tiempo                                                              */
/* ------------------------------------------------------------------ */

long mios_time(long *tloc) {
    return sys_ret(mios_syscall1(SYS_time, (long)tloc));
}

int mios_gettimeofday(struct mios_timeval *tv, void *tz) {
    return (int)sys_ret(mios_syscall2(SYS_gettimeofday, (long)tv, (long)tz));
}

int mios_clock_gettime(int clockid, struct mios_timespec *ts) {
    return (int)sys_ret(mios_syscall2(SYS_clock_gettime, clockid, (long)ts));
}

int mios_clock_getres(int clockid, struct mios_timespec *ts) {
    return (int)sys_ret(mios_syscall2(SYS_clock_getres, clockid, (long)ts));
}

unsigned int mios_alarm(unsigned int seconds) {
    return (unsigned int)sys_ret(mios_syscall1(SYS_alarm, seconds));
}

/* ------------------------------------------------------------------ */
/* Sockets y red                                                       */
/* ------------------------------------------------------------------ */

int mios_socket(int domain, int type, int protocol) {
    return (int)sys_ret(mios_syscall3(SYS_socket, domain, type, protocol));
}

int mios_bind(int fd, const void *addr, unsigned int addrlen) {
    return (int)sys_ret(mios_syscall3(SYS_bind, fd, (long)addr, (long)addrlen));
}

long mios_sendto(int fd, const void *buf, mios_size_t len, int flags,
                 const void *addr, unsigned int addrlen) {
    return sys_ret(mios_syscall6(SYS_sendto, fd, (long)buf, (long)len, flags,
                                 (long)addr, (long)addrlen));
}

long mios_recvfrom(int fd, void *buf, mios_size_t len, int flags,
                   void *addr, unsigned int *addrlen) {
    return sys_ret(mios_syscall6(SYS_recvfrom, fd, (long)buf, (long)len, flags,
                                 (long)addr, (long)addrlen));
}

/* ------------------------------------------------------------------ */
/* Modulos del kernel                                                  */
/* ------------------------------------------------------------------ */

int mios_finit_module(int fd, const char *params, int flags) {
    return (int)sys_ret(mios_syscall3(SYS_finit_module, fd, (long)params, flags));
}

int mios_getrandom(void *buf, mios_size_t len, unsigned int flags) {
    return (int)sys_ret(mios_syscall3(SYS_getrandom, (long)buf, (long)len, flags));
}

/* ------------------------------------------------------------------ */
/* Limites y consumo de recursos (Fase 7: los pide binutils)           */
/* ------------------------------------------------------------------ */

int mios_getrusage(int who, void *usage) {
    return (int)sys_ret(mios_syscall2(SYS_getrusage, who, (long)usage));
}

int mios_getrlimit(int resource, void *rlim) {
    return (int)sys_ret(mios_syscall2(SYS_getrlimit, resource, (long)rlim));
}

int mios_setrlimit(int resource, const void *rlim) {
    return (int)sys_ret(mios_syscall2(SYS_setrlimit, resource, (long)rlim));
}
