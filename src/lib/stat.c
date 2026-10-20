#include "lib/lib.h"
#include "lib/ret.h"
#include "kernel/syscall.h"
#include "../../include/mios.h"

/*
 * MiOS - metadatos de ficheros y descriptores (sys/stat.h, unistd.h, fcntl.h).
 */

/* ------------------------------------------------------------------ */
/* stat / fstat / lstat                                                */
/* ------------------------------------------------------------------ */

int stat(const char *path, struct stat *st) {
    struct mios_stat k;

    if (mios_stat(path, &k) < 0) {
        return -1;
    }
    memcpy(st, &k, sizeof(k));
    return 0;
}

int fstat(int fd, struct stat *st) {
    struct mios_stat k;

    if (mios_fstat(fd, &k) < 0) {
        return -1;
    }
    memcpy(st, &k, sizeof(k));
    return 0;
}

int lstat(const char *path, struct stat *st) {
    struct mios_stat k;

    if (mios_lstat(path, &k) < 0) {
        return -1;
    }
    memcpy(st, &k, sizeof(k));
    return 0;
}

/* ------------------------------------------------------------------ */
/* Permisos                                                            */
/* ------------------------------------------------------------------ */

int chmod(const char *path, mode_t mode) {
    return (int)mios_ret(mios_chmod(path, (int)mode));
}

int fchmod(int fd, mode_t mode) {
    return (int)mios_ret(mios_fchmod(fd, (int)mode));
}

int chown(const char *path, int uid, int gid) {
    return (int)mios_ret(mios_chown(path, uid, gid));
}

int umask(mode_t mask) {
    return (int)mios_ret(mios_umask((int)mask));
}

int access(const char *path, int mode) {
    return (int)mios_ret(mios_access(path, mode));
}

/* ------------------------------------------------------------------ */
/* Descriptores                                                        */
/* ------------------------------------------------------------------ */

off_t lseek(int fd, off_t offset, int whence) {
    return (off_t)mios_ret(mios_lseek(fd, (long)offset, whence));
}

int dup(int fd) {
    return (int)mios_ret(mios_dup(fd));
}

int dup2(int oldfd, int newfd) {
    return (int)mios_ret(mios_dup2(oldfd, newfd));
}

int pipe(int *fds) {
    return (int)mios_ret(mios_pipe(fds));
}

int ioctl(int fd, unsigned long request, ...) {
    __builtin_va_list ap;
    void *arg;

    __builtin_va_start(ap, request);
    arg = __builtin_va_arg(ap, void *);
    __builtin_va_end(ap);
    return (int)mios_ret(mios_ioctl(fd, request, arg));
}

int fcntl(int fd, int cmd, ...) {
    __builtin_va_list ap;
    long arg = 0;

    __builtin_va_start(ap, cmd);
    arg = __builtin_va_arg(ap, long);
    __builtin_va_end(ap);
    return (int)mios_ret(mios_fcntl(fd, cmd, arg));
}

int isatty(int fd) {
    char termios_buf[64];

    /* TCGETS = 0x5401 (el kernel espera struct termios) */
    return mios_ioctl(fd, 0x5401, termios_buf) == 0;
}

ssize_t pread(int fd, void *buf, size_t count, off_t offset) {
    return (ssize_t)mios_ret(mios_pread(fd, buf, (mios_size_t)count,
                                        (long)offset));
}

ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset) {
    return (ssize_t)mios_ret(mios_pwrite(fd, buf, (mios_size_t)count,
                                         (long)offset));
}

int truncate(const char *path, off_t length) {
    return (int)mios_ret(mios_truncate(path, (long)length));
}

int ftruncate(int fd, off_t length) {
    return (int)mios_ret(mios_ftruncate(fd, (long)length));
}

int fsync(int fd) {
    return (int)mios_ret(mios_fsync(fd));
}

/* ------------------------------------------------------------------ */
/* Enlaces                                                             */
/* ------------------------------------------------------------------ */

int link(const char *oldpath, const char *newpath) {
    return (int)mios_ret(mios_link(oldpath, newpath));
}

int symlink(const char *target, const char *linkpath) {
    return (int)mios_ret(mios_symlink(target, linkpath));
}

ssize_t readlink(const char *path, char *buf, size_t size) {
    return (ssize_t)mios_ret(mios_readlink(path, buf, (mios_size_t)size));
}

int creat(const char *path, mode_t mode) {
    return mios_open(path, MIOS_O_WRONLY | MIOS_O_CREAT | MIOS_O_TRUNC,
                     (int)mode);
}
