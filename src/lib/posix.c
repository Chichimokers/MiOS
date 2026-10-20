#include "lib/lib.h"
#include "kernel/syscall.h"
#include "../../include/mios.h"   /* cabeceras publicas: comprobacion de firmas */

/*
 * MiOS - nombres "estandar" sobre la API interna.
 *
 * libmios exporta sus funciones con nombres propios (mios_read, cons_printf,
 * mios_mkdir...).  Aqui se anaden los nombres habituales de C/POSIX
 * (read, printf, open, mkdir, exit, strlen...) para que un programa normal no
 * tenga que conocer los mios_*, y para que libmios sea compatible con el
 * codigo que genera gcc.
 *
 * Convenio de errores: el kernel devuelve -errno; estas funciones lo traducen
 * a -1 y dejan el codigo en errno.
 *
 * errno se obtiene con __errno_location() (vive en src/lib/errno.c) y NO como
 * variable global: asi el ejecutable no necesita una relocalizacion
 * R_X86_64_COPY solo para leerlo.
 */

/* Normaliza un resultado del kernel: -errno pasa a -1 y errno. */
static long posix_ret(long ret) {
    int err = mios_error(ret);

    if (err != 0) {
        *__errno_location() = err;
        return -1;
    }
    return ret;
}

/* ------------------------------------------------------------------ */
/* Consola: los printf/puts reales viven en src/lib/stdio.c            */
/* ------------------------------------------------------------------ */

/* ------------------------------------------------------------------ */
/* Ficheros y directorios                                              */
/* ------------------------------------------------------------------ */

int open(const char *path, int flags, ...) {
    int mode = 0;

    if (flags & (0100 | 0200)) { /* O_CREAT | O_EXCL */
        __builtin_va_list ap;

        __builtin_va_start(ap, flags);
        mode = __builtin_va_arg(ap, int);
        __builtin_va_end(ap);
    }
    return (int)posix_ret(mios_open(path, flags, mode));
}

ssize_t read(int fd, void *buf, size_t count) {
    return (ssize_t)posix_ret(mios_read(fd, buf, (mios_size_t)count));
}

ssize_t write(int fd, const void *buf, size_t count) {
    return (ssize_t)posix_ret(mios_write(fd, buf, (mios_size_t)count));
}

int close(int fd) {
    return (int)posix_ret(mios_close(fd));
}

int unlink(const char *path) {
    return (int)posix_ret(mios_unlink(path));
}

int chdir(const char *path) {
    return (int)posix_ret(mios_chdir(path));
}

char *getcwd(char *buf, size_t size) {
    if (posix_ret(mios_getcwd(buf, (mios_size_t)size)) < 0) {
        return MIOS_NULL;
    }
    return buf;
}

int mkdir(const char *path, mode_t mode) {
    return (int)posix_ret(mios_mkdir(path, (int)mode));
}

int rmdir(const char *path) {
    return (int)posix_ret(mios_rmdir(path));
}

int rename(const char *oldpath, const char *newpath) {
    return (int)posix_ret(mios_rename(oldpath, newpath));
}

int mknod(const char *path, int mode, unsigned long dev) {
    return (int)posix_ret(mios_mknod(path, mode, dev));
}

int mount(const char *src, const char *target, const char *fstype,
          unsigned long flags, const void *data) {
    return (int)posix_ret(mios_mount(src, target, fstype, flags, data));
}

/* ------------------------------------------------------------------ */
/* Sistema                                                             */
/* ------------------------------------------------------------------ */

int uname(struct utsname *u) {
    return (int)posix_ret(mios_uname((struct mios_utsname *)u));
}

int getpid(void) {
    return mios_getpid();
}

int getuid(void) {
    return mios_getuid();
}

int getgid(void) {
    return mios_getgid();
}

unsigned int sleep(unsigned int seconds) {
    mios_sleep((long)seconds);
    return 0;
}

void reboot(int magic1, int magic2, int cmd) {
    mios_reboot(magic1, magic2, cmd);
}

const char *strerror(int err) {
    return mios_strerror(err);
}

/* ------------------------------------------------------------------ */
/* Cadenas                                                             */
/* ------------------------------------------------------------------ */

size_t strlen(const char *s) {
    return (size_t)mios_strlen(s);
}

int strcmp(const char *a, const char *b) {
    return mios_strcmp(a, b);
}

int strncmp(const char *a, const char *b, size_t n) {
    return mios_strncmp(a, b, (mios_size_t)n);
}

char *strcpy(char *dst, const char *src) {
    return mios_strcpy(dst, src);
}

char *strncpy(char *dst, const char *src, size_t n) {
    return mios_strncpy(dst, src, (mios_size_t)n);
}

char *strchr(const char *s, int c) {
    return mios_strchr(s, c);
}

/* ------------------------------------------------------------------ */
/* Memoria: mmap/munmap/mprotect/msync (las de Linux x86_64)          */
/* ------------------------------------------------------------------ */

/* mios_mmap ya devuelve MAP_FAILED y deja errno puesto. */
void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off) {
    return mios_mmap(addr, (mios_size_t)len, prot, flags, fd, (long)off);
}

int munmap(void *addr, size_t len) {
    return (int)posix_ret(mios_munmap(addr, (mios_size_t)len));
}

int mprotect(void *addr, size_t len, int prot) {
    return (int)posix_ret(mios_mprotect(addr, (mios_size_t)len, prot));
}

int msync(void *addr, size_t len, int flags) {
    return (int)posix_ret(mios_msync(addr, (mios_size_t)len, flags));
}

/* ------------------------------------------------------------------ */
/* Conversion: vive en src/lib/stdlib.c                                */
/* ------------------------------------------------------------------ */
