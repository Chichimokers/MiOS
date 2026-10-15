#include "mios.h"

/*
 * MiOS - operaciones de archivos de alto nivel.
 */

long file_read(const char *path, char *buf, mios_size_t max) {
    int fd;
    long total = 0;

    if (max == 0) {
        return -MIOS_EINVAL;
    }

    fd = mios_open(path, MIOS_O_RDONLY, 0);
    if (fd < 0) {
        return fd;
    }

    while ((mios_size_t)total < max - 1) {
        long r = mios_read(fd, buf + total, (max - 1) - (mios_size_t)total);
        if (r < 0) {
            mios_close(fd);
            return r;
        }
        if (r == 0) {
            break;
        }
        total += r;
    }

    buf[total] = '\0';
    mios_close(fd);
    return total;
}

void file_cat(const char *path) {
    int fd;
    char buf[512];
    long r;

    fd = mios_open(path, MIOS_O_RDONLY, 0);
    if (fd < 0) {
        cons_printf("cat: %s: %s\n", path, mios_strerror(mios_error(fd)));
        return;
    }

    while ((r = mios_read(fd, buf, sizeof(buf))) > 0) {
        mios_write(1, buf, (mios_size_t)r);
    }

    mios_close(fd);
}

int file_copy(const char *src, const char *dst) {
    int in;
    int out;
    char buf[1024];
    long r;

    in = mios_open(src, MIOS_O_RDONLY, 0);
    if (in < 0) {
        return in;
    }

    out = mios_open(dst, MIOS_O_WRONLY | MIOS_O_CREAT | MIOS_O_TRUNC, 0644);
    if (out < 0) {
        mios_close(in);
        return out;
    }

    while ((r = mios_read(in, buf, sizeof(buf))) > 0) {
        long written = 0;
        while (written < r) {
            long w = mios_write(out, buf + written, (mios_size_t)(r - written));
            if (w < 0) {
                mios_close(in);
                mios_close(out);
                return (int)w;
            }
            written += w;
        }
    }

    mios_close(in);
    mios_close(out);

    if (r < 0) {
        return (int)r;
    }
    return 0;
}
