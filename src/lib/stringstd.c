#include "lib/lib.h"
#include "kernel/syscall.h"
#include "../../include/mios.h"

/*
 * MiOS - string.h: las funciones que no estaban ya en posix.c
 * (strlen, strcmp, strncmp, strcpy, strncpy y strchr viven alli).
 */

char *strcat(char *dst, const char *src) {
    char *d = dst + strlen(dst);

    while (*src != '\0') {
        *d = *src;
        d++;
        src++;
    }
    *d = '\0';
    return dst;
}

char *strncat(char *dst, const char *src, size_t n) {
    char *d = dst + strlen(dst);

    while (n > 0 && *src != '\0') {
        *d = *src;
        d++;
        src++;
        n--;
    }
    *d = '\0';
    return dst;
}

char *stpcpy(char *dst, const char *src) {
    while (*src != '\0') {
        *dst = *src;
        dst++;
        src++;
    }
    *dst = '\0';
    return dst;
}

char *strrchr(const char *s, int c) {
    const char *last = (const char *)0;

    for (;;) {
        if (*s == (char)c) {
            last = s;
        }
        if (*s == '\0') {
            break;
        }
        s++;
    }
    return (char *)last;
}

void *memchr(const void *s, int c, size_t n) {
    const unsigned char *p = (const unsigned char *)s;

    while (n-- > 0) {
        if (*p == (unsigned char)c) {
            return (void *)p;
        }
        p++;
    }
    return (void *)0;
}

char *strstr(const char *haystack, const char *needle) {
    if (*needle == '\0') {
        return (char *)haystack;
    }
    for (; *haystack != '\0'; haystack++) {
        const char *h = haystack;
        const char *n = needle;

        while (*h != '\0' && *n != '\0' && *h == *n) {
            h++;
            n++;
        }
        if (*n == '\0') {
            return (char *)haystack;
        }
    }
    return (char *)0;
}

size_t strspn(const char *s, const char *accept) {
    size_t n = 0;

    while (s[n] != '\0' && strchr(accept, s[n]) != (char *)0) {
        n++;
    }
    return n;
}

size_t strcspn(const char *s, const char *reject) {
    size_t n = 0;

    while (s[n] != '\0' && strchr(reject, s[n]) == (char *)0) {
        n++;
    }
    return n;
}

char *strpbrk(const char *s, const char *accept) {
    while (*s != '\0') {
        if (strchr(accept, *s) != (char *)0) {
            return (char *)s;
        }
        s++;
    }
    return (char *)0;
}

char *strtok_r(char *s, const char *delim, char **saveptr) {
    char *start;

    if (s == (char *)0) {
        s = *saveptr;
    }
    if (s == (char *)0) {
        return (char *)0;
    }

    s += strspn(s, delim);
    if (*s == '\0') {
        *saveptr = s;
        return (char *)0;
    }

    start = s;
    s += strcspn(s, delim);
    if (*s != '\0') {
        *s = '\0';
        s++;
    }
    *saveptr = s;
    return start;
}

char *strtok(char *s, const char *delim) {
    static char *save = (char *)0;

    return strtok_r(s, delim, &save);
}

char *strdup(const char *s) {
    size_t n = strlen(s) + 1;
    char *p = (char *)malloc(n);

    if (p != (char *)0) {
        memcpy(p, s, n);
    }
    return p;
}

char *strndup(const char *s, size_t n) {
    size_t len = strnlen(s, n);
    char *p = (char *)malloc(len + 1);

    if (p != (char *)0) {
        memcpy(p, s, len);
        p[len] = '\0';
    }
    return p;
}

size_t strnlen(const char *s, size_t maxlen) {
    size_t n = 0;

    while (n < maxlen && s[n] != '\0') {
        n++;
    }
    return n;
}

static int lower(int c) {
    return (c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c;
}

int strcasecmp(const char *a, const char *b) {
    for (;;) {
        int ca = lower((unsigned char)*a);
        int cb = lower((unsigned char)*b);

        if (ca != cb) {
            return ca - cb;
        }
        if (ca == 0) {
            return 0;
        }
        a++;
        b++;
    }
}

int strncasecmp(const char *a, const char *b, size_t n) {
    while (n > 0) {
        int ca = lower((unsigned char)*a);
        int cb = lower((unsigned char)*b);

        if (ca != cb) {
            return ca - cb;
        }
        if (ca == 0) {
            return 0;
        }
        a++;
        b++;
        n--;
    }
    return 0;
}

/* Como strchr(), pero si no esta devuelve el puntero al '\0' final. */
char *strchrnul(const char *s, int c) {
    char ch = (char)c;

    while (*s != '\0' && *s != ch) {
        s++;
    }
    return (char *)s;
}

/* Como memcpy(), pero devuelve el puntero al byte siguiente al copiado. */
void *mempcpy(void *dst, const void *src, size_t n) {
    memcpy(dst, src, n);
    return (char *)dst + n;
}

/* ------------------------------------------------------------------ */
/* Ordenacion segun la localizacion (Fase 7)                           */
/* ------------------------------------------------------------------ */
/* MiOS solo tiene la localizacion "C", asi que strcoll() es strcmp() y
 * strxfrm() es strcpy().  Los usa binutils para ordenar tablas de simbolos
 * (binutils/nm.c) y para comparar nombres de fichero. */

int strcoll(const char *a, const char *b) {
    return strcmp(a, b);
}

size_t strxfrm(char *dst, const char *src, size_t n) {
    size_t len = strlen(src);

    if (n > 0) {
        size_t copia = (len < n - 1) ? len : n - 1;

        memcpy(dst, src, copia);
        dst[copia] = '\0';
    }
    return len;
}
