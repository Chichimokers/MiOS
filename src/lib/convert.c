#include "mios.h"

/*
 * MiOS - conversion entre texto y numeros.
 */

int mios_digit(int c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

long mios_atol(const char *s) {
    long value = 0;
    int negative = 0;

    while (*s == ' ' || *s == '\t' || *s == '\n') {
        s++;
    }
    if (*s == '-') {
        negative = 1;
        s++;
    } else if (*s == '+') {
        s++;
    }
    while (*s >= '0' && *s <= '9') {
        value = value * 10 + (*s - '0');
        s++;
    }
    return negative ? -value : value;
}

int mios_atoi(const char *s) {
    return (int)mios_atol(s);
}

unsigned long mios_strtoul(const char *s, int base) {
    unsigned long value = 0;
    int d;

    while (*s == ' ' || *s == '\t') {
        s++;
    }
    if (base == 0) {
        base = 10;
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
            base = 16;
            s += 2;
        }
    }

    for (;;) {
        d = mios_digit(*s);
        if (d < 0 || d >= base) {
            break;
        }
        value = value * (unsigned long)base + (unsigned long)d;
        s++;
    }
    return value;
}
