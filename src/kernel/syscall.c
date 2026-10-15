#include "mios.h"

/*
 * MiOS - capa de llamadas al sistema (x86_64).
 *
 * Estas son las unicas funciones que hablan con el kernel.
 * Los numeros de syscall corresponden a Linux x86_64.
 */

 
#define SYS_read        0
#define SYS_write       1
#define SYS_open        2
#define SYS_close       3
#define SYS_nanosleep   35
#define SYS_getpid      39
#define SYS_socket      41
#define SYS_sendto      44
#define SYS_recvfrom    45
#define SYS_bind        49
#define SYS_uname       63
#define SYS_getcwd      79
#define SYS_chdir       80
#define SYS_rename      82
#define SYS_mkdir       83
#define SYS_rmdir       84
#define SYS_unlink      87
#define SYS_getuid      102
#define SYS_getgid      104
#define SYS_mount       165
#define SYS_reboot      169
#define SYS_getdents64  217
#define SYS_exit_group  231
#define SYS_mknodat     259
#define SYS_finit_module 313

#define MIOS_AT_FDCWD (-100)

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
    return mios_syscall3(SYS_read, fd, (long)buf, (long)count);
}

long mios_write(int fd, const void *buf, mios_size_t count) {
    return mios_syscall3(SYS_write, fd, (long)buf, (long)count);
}

int mios_open(const char *path, int flags, int mode) {
    return (int)mios_syscall3(SYS_open, (long)path, flags, mode);
}

int mios_close(int fd) {
    return (int)mios_syscall1(SYS_close, fd);
}

long mios_getdents64(int fd, void *buf, mios_size_t count) {
    return mios_syscall3(SYS_getdents64, fd, (long)buf, (long)count);
}

long mios_mount(const char *src, const char *target, const char *fstype,
                unsigned long flags, const void *data) {
    return mios_syscall5(SYS_mount, (long)src, (long)target, (long)fstype,
                         (long)flags, (long)data);
}

int mios_mkdir(const char *path, int mode) {
    return (int)mios_syscall2(SYS_mkdir, (long)path, mode);
}

int mios_rmdir(const char *path) {
    return (int)mios_syscall1(SYS_rmdir, (long)path);
}

int mios_chdir(const char *path) {
    return (int)mios_syscall1(SYS_chdir, (long)path);
}

long mios_getcwd(char *buf, mios_size_t size) {
    return mios_syscall2(SYS_getcwd, (long)buf, (long)size);
}

int mios_rename(const char *oldpath, const char *newpath) {
    return (int)mios_syscall2(SYS_rename, (long)oldpath, (long)newpath);
}

int mios_unlink(const char *path) {
    return (int)mios_syscall1(SYS_unlink, (long)path);
}

int mios_mknod(const char *path, int mode, unsigned long dev) {
    return (int)mios_syscall4(SYS_mknodat, MIOS_AT_FDCWD, (long)path,
                              mode, (long)dev);
}

int mios_uname(struct mios_utsname *u) {
    return (int)mios_syscall1(SYS_uname, (long)u);
}

int mios_getpid(void) {
    return (int)mios_syscall0(SYS_getpid);
}

int mios_getuid(void) {
    return (int)mios_syscall0(SYS_getuid);
}

int mios_getgid(void) {
    return (int)mios_syscall0(SYS_getgid);
}

int mios_reboot(int magic1, int magic2, int cmd) {
    return (int)mios_syscall4(SYS_reboot, magic1, magic2, cmd, 0);
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
    mios_syscall2(SYS_nanosleep, (long)&ts, 0);
}

int mios_socket(int domain, int type, int protocol) {
    return (int)mios_syscall3(SYS_socket, domain, type, protocol);
}

int mios_bind(int fd, const void *addr, unsigned int addrlen) {
    return (int)mios_syscall3(SYS_bind, fd, (long)addr, (long)addrlen);
}

long mios_sendto(int fd, const void *buf, mios_size_t len, int flags,
                 const void *addr, unsigned int addrlen) {
    return mios_syscall6(SYS_sendto, fd, (long)buf, (long)len, flags,
                         (long)addr, (long)addrlen);
}

long mios_recvfrom(int fd, void *buf, mios_size_t len, int flags,
                   void *addr, unsigned int *addrlen) {
    return mios_syscall6(SYS_recvfrom, fd, (long)buf, (long)len, flags,
                         (long)addr, (long)addrlen);
}

int mios_finit_module(int fd, const char *params, int flags) {
    return (int)mios_syscall3(SYS_finit_module, fd, (long)params, flags);
}
