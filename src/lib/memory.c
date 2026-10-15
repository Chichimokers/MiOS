#include "mios.h"

/*
 * MiOS - utilidades de memoria.
 */

void *mios_memset(void *dst, int c, mios_size_t n) {
    unsigned char *d = (unsigned char *)dst;
    while (n > 0) {
        *d = (unsigned char)c;
        d++;
        n--;
    }
    return dst;
}

void *mios_memcpy(void *dst, const void *src, mios_size_t n) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    while (n > 0) {
        *d = *s;
        d++;
        s++;
        n--;
    }
    return dst;
}

void *mios_memmove(void *dst, const void *src, mios_size_t n) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    if (d < s) {
        while (n > 0) {
            *d = *s;
            d++;
            s++;
            n--;
        }
    } else {
        d += n;
        s += n;
        while (n > 0) {
            d--;
            s--;
            *d = *s;
            n--;
        }
    }
    return dst;
}

int mios_memcmp(const void *a, const void *b, mios_size_t n) {
    const unsigned char *x = (const unsigned char *)a;
    const unsigned char *y = (const unsigned char *)b;
    while (n > 0) {
        if (*x != *y) {
            return (int)*x - (int)*y;
        }
        x++;
        y++;
        n--;
    }
    return 0;
}

/*
 * Alias con nombres estandar: el compilador puede generar llamadas a
 * memset/memcpy en inicializaciones aunque no exista libc.
 */

void *memset(void *dst, int c, mios_size_t n) {
    return mios_memset(dst, c, n);
}

void *memcpy(void *dst, const void *src, mios_size_t n) {
    return mios_memcpy(dst, src, n);
}

void *memmove(void *dst, const void *src, mios_size_t n) {
    return mios_memmove(dst, src, n);
}

int memcmp(const void *a, const void *b, mios_size_t n) {
    return mios_memcmp(a, b, n);
}
