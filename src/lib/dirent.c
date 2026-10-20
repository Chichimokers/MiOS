#include "lib/lib.h"
#include "lib/ret.h"
#include "kernel/syscall.h"
#include "../../include/mios.h"

/*
 * MiOS - dirent.h: lectura de directorios con getdents64.
 *
 * Un DIR tiene su propio bloque de lectura (sin malloc por cada readdir).
 */

#define DIR_BUFSZ 4096

struct _mios_DIR {
    int    fd;
    long   pos;   /* siguiente entrada dentro del bloque */
    long   used;  /* bytes validos del bloque */
    long   tell;  /* posicion logica (telldir) */
    struct dirent ent;
    char   buf[DIR_BUFSZ];
};

DIR *opendir(const char *path) {
    int fd;
    DIR *d;

    fd = mios_open(path, MIOS_O_RDONLY | MIOS_O_DIRECTORY, 0);
    if (fd < 0) {
        return (DIR *)0;
    }

    d = (DIR *)malloc(sizeof(DIR));
    if (d == (DIR *)0) {
        mios_close(fd);
        return (DIR *)0;
    }
    memset(d, 0, sizeof(DIR));
    d->fd = fd;
    return d;
}

struct dirent *readdir(DIR *d) {
    if (d == (DIR *)0) {
        return (struct dirent *)0;
    }

    for (;;) {
        struct mios_dirent64 *e;
        const char *name;
        int i;

        if (d->pos >= d->used) {
            long r = mios_getdents64(d->fd, d->buf, DIR_BUFSZ);

            if (r <= 0) {
                return (struct dirent *)0;
            }
            d->used = r;
            d->pos = 0;
        }

        e = (struct mios_dirent64 *)(void *)(d->buf + d->pos);
        d->pos += e->d_reclen;
        if (e->d_ino == 0 || e->d_reclen == 0) {
            continue;
        }

        d->ent.d_ino = e->d_ino;
        d->ent.d_off = e->d_off;
        d->ent.d_reclen = e->d_reclen;
        d->ent.d_type = e->d_type;
        name = e->d_name;
        for (i = 0; i < 255 && name[i] != '\0'; i++) {
            d->ent.d_name[i] = name[i];
        }
        d->ent.d_name[i] = '\0';
        d->tell = e->d_off;
        return &d->ent;
    }
}

int closedir(DIR *d) {
    int rc;

    if (d == (DIR *)0) {
        errno = EBADF;
        return -1;
    }
    rc = (int)mios_ret(mios_close(d->fd));
    free(d);
    return rc;
}

void rewinddir(DIR *d) {
    if (d == (DIR *)0) {
        return;
    }
    mios_lseek(d->fd, 0, MIOS_SEEK_SET);
    d->pos = 0;
    d->used = 0;
    d->tell = 0;
}

long telldir(DIR *d) {
    return (d != (DIR *)0) ? d->tell : -1;
}

void seekdir(DIR *d, long loc) {
    if (d == (DIR *)0) {
        return;
    }
    mios_lseek(d->fd, loc, MIOS_SEEK_SET);
    d->pos = 0;
    d->used = 0;
    d->tell = loc;
}
