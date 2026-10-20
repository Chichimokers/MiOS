#ifndef MIOS_KERNEL_SYSCALL_H
#define MIOS_KERNEL_SYSCALL_H

#include "lib/types.h"
#include "kernel/kernel.h"

/* Primitivas de syscall (src/kernel/syscall.c) */


long mios_syscall0(long n);
long mios_syscall1(long n, long a1);
long mios_syscall2(long n, long a1, long a2);
long mios_syscall3(long n, long a1, long a2, long a3);
long mios_syscall4(long n, long a1, long a2, long a3, long a4);
long mios_syscall5(long n, long a1, long a2, long a3, long a4, long a5);
long mios_syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6);

int mios_error(long ret);

/* errno (el valor global vive en src/lib/errno.c) */
int *__errno_location(void);

/* Archivos y procesos */
long mios_read(int fd, void *buf, mios_size_t count);
long mios_write(int fd, const void *buf, mios_size_t count);
long mios_pread(int fd, void *buf, mios_size_t count, long off);
long mios_pwrite(int fd, const void *buf, mios_size_t count, long off);
int  mios_open(const char *path, int flags, int mode);
int  mios_close(int fd);
long mios_lseek(int fd, long offset, int whence);
long mios_getdents64(int fd, void *buf, mios_size_t count);
int  mios_mount(const char *src, const char *target, const char *fstype,
                unsigned long flags, const void *data);
int  mios_mkdir(const char *path, int mode);
int  mios_rmdir(const char *path);
int  mios_chdir(const char *path);
long mios_getcwd(char *buf, mios_size_t size);
int  mios_rename(const char *oldpath, const char *newpath);
int  mios_unlink(const char *path);
int  mios_mknod(const char *path, int mode, unsigned long dev);
int  mios_uname(struct mios_utsname *u);
int  mios_getpid(void);
int  mios_getppid(void);
int  mios_getuid(void);
int  mios_getgid(void);
int  mios_reboot(int magic1, int magic2, int cmd);
void mios_exit(int code) __attribute__((noreturn));
void mios_sleep(long seconds);
int  mios_sleep_ns(long seconds, long nanoseconds);

/* Memoria */
void *mios_mmap(void *addr, mios_size_t len, int prot, int flags, int fd, long off);
int   mios_munmap(void *addr, mios_size_t len);
int   mios_mprotect(void *addr, mios_size_t len, int prot);
int   mios_msync(void *addr, mios_size_t len, int flags);
long  mios_brk(void *addr);

/* Ficheros: metadatos */
int  mios_fstat(int fd, struct mios_stat *st);
int  mios_stat(const char *path, struct mios_stat *st);
int  mios_lstat(const char *path, struct mios_stat *st);
int  mios_access(const char *path, int mode);
int  mios_chmod(const char *path, int mode);
int  mios_fchmod(int fd, int mode);
int  mios_fsync(int fd);
int  mios_sigprocmask(int how, const void *set, void *oldset);
int  mios_chown(const char *path, int uid, int gid);
int  mios_utime(const char *path, const struct mios_utimbuf *times);
int  mios_link(const char *oldpath, const char *newpath);
int  mios_symlink(const char *target, const char *linkpath);
long mios_readlink(const char *path, char *buf, mios_size_t size);
int  mios_umask(int mask);
int  mios_ftruncate(int fd, long length);
int  mios_truncate(const char *path, long length);

/* Descriptores */
int  mios_dup(int fd);
int  mios_dup2(int oldfd, int newfd);
int  mios_pipe(int *fds);
int  mios_fcntl(int fd, int cmd, long arg);
int  mios_ioctl(int fd, unsigned long request, void *arg);

/* Procesos */
int  mios_fork(void);
int  mios_execve(const char *path, char *const argv[], char *const envp[]);
int  mios_wait4(int pid, int *status, int options, void *rusage);
int  mios_kill(int pid, int sig);
int  mios_setsid(void);
int  mios_setpgid(int pid, int pgid);
int  mios_getpgid(int pid);
int  mios_sigaction(int sig, const void *act, void *oldact);

/* Tiempo */
long mios_time(long *tloc);
int  mios_gettimeofday(struct mios_timeval *tv, void *tz);
int  mios_clock_gettime(int clockid, struct mios_timespec *ts);
int  mios_clock_getres(int clockid, struct mios_timespec *ts);
unsigned int mios_alarm(unsigned int seconds);

/* Limites y consumo de recursos (Fase 7) */
int  mios_getrusage(int who, void *usage);
int  mios_getrlimit(int resource, void *rlim);
int  mios_setrlimit(int resource, const void *rlim);

/* Sockets y red */
int  mios_socket(int domain, int type, int protocol);
int  mios_bind(int fd, const void *addr, unsigned int addrlen);
long mios_sendto(int fd, const void *buf, mios_size_t len, int flags,
                 const void *addr, unsigned int addrlen);
long mios_recvfrom(int fd, void *buf, mios_size_t len, int flags,
                   void *addr, unsigned int *addrlen);

/* Modulos del kernel */
int  mios_finit_module(int fd, const char *params, int flags);

#endif /* MIOS_KERNEL_SYSCALL_H */
