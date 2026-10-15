#include "mios.h"

/*
 * MiOS - utilidades de texto.
 * Implementadas a mano: el sistema no enlaza libc.
 */

mios_size_t mios_strlen(const char *s) {
    mios_size_t n = 0;
    while (s[n] != '\0') {
        n++;
    }
    return n;
}

int mios_strcmp(const char *a, const char *b) {
    while (*a != '\0' && *a == *b) {
        a++;
        b++;
    }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

int mios_strncmp(const char *a, const char *b, mios_size_t n) {
    mios_size_t i = 0;
    while (i < n && a[i] != '\0' && a[i] == b[i]) {
        i++;
    }
    if (i == n) {
        return 0;
    }
    return (int)(unsigned char)a[i] - (int)(unsigned char)b[i];
}

char *mios_strcpy(char *dst, const char *src) {
    char *d = dst;
    while ((*d = *src) != '\0') {
        d++;
        src++;
    }
    return dst;
}

char *mios_strncpy(char *dst, const char *src, mios_size_t n) {
    mios_size_t i = 0;
    while (i < n && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    while (i < n) {
        dst[i] = '\0';
        i++;
    }
    return dst;
}

char *mios_strchr(const char *s, int c) {
    for (;;) {
        if (*s == (char)c) {
            return (char *)s;
        }
        if (*s == '\0') {
            return MIOS_NULL;
        }
        s++;
    }
}

int mios_split_words(char *s, char **tokens, int max) {
    int count = 0;
    char *p = s;
    while (count < max) {
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        if (*p == '\0') {
            break;
        }
        tokens[count] = p;
        count++;
        while (*p != '\0' && *p != ' ' && *p != '\t') {
            p++;
        }
        if (*p != '\0') {
            *p = '\0';
            p++;
        }
    }
    return count;
}
