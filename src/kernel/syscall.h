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

/* Archivos y procesos */
long mios_read(int fd, void *buf, mios_size_t count);
long mios_write(int fd, const void *buf, mios_size_t count);
int  mios_open(const char *path, int flags, int mode);
int  mios_close(int fd);
long mios_getdents64(int fd, void *buf, mios_size_t count);
long mios_mount(const char *src, const char *target, const char *fstype,
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
int  mios_getuid(void);
int  mios_getgid(void);
int  mios_reboot(int magic1, int magic2, int cmd);
void mios_exit(int code) __attribute__((noreturn));
void mios_sleep(long seconds);

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
