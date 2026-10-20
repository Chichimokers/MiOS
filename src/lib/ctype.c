#include "lib/lib.h"
#include "../../include/mios.h"

/*
 * MiOS - ctype.h.
 *
 * Funciones normales (no macros) para que el codigo portado no dependa de la
 * tabla _ctype_ de glibc.
 */

int isspace(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' ||
           c == '\r';
}

int isdigit(int c) {
    return c >= '0' && c <= '9';
}

int isxdigit(int c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
}

int isupper(int c) {
    return c >= 'A' && c <= 'Z';
}

int islower(int c) {
    return c >= 'a' && c <= 'z';
}

int isalpha(int c) {
    return isupper(c) || islower(c);
}

int isalnum(int c) {
    return isalpha(c) || isdigit(c);
}

int isprint(int c) {
    return c >= 0x20 && c < 0x7f;
}

int isgraph(int c) {
    return c > 0x20 && c < 0x7f;
}

int ispunct(int c) {
    return isgraph(c) && !isalnum(c);
}

int iscntrl(int c) {
    return (c >= 0 && c < 0x20) || c == 0x7f;
}

int isblank(int c) {
    return c == ' ' || c == '\t';
}

int isascii(int c) {
    return c >= 0 && c <= 0x7f;
}

int toascii(int c) {
    return c & 0x7f;
}

int toupper(int c) {
    return islower(c) ? (c - 'a' + 'A') : c;
}

int tolower(int c) {
    return isupper(c) ? (c - 'A' + 'a') : c;
}
