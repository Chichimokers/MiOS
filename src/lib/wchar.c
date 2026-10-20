#include "lib/lib.h"
#include "kernel/syscall.h"
#include "../../include/mios.h"
#include "../../include/wchar.h"

/*
 * MiOS - caracteres anchos (wchar.h).
 *
 * MiOS trabaja con bytes: no hay locales ni juegos de caracteres alternativos,
 * asi que la conversion multibyte <-> ancho es la identidad byte a byte (cada
 * byte es un caracter ancho de valor 0..255).  Es lo que necesitan los que
 * piden estas funciones: gas usa mbstowcs(NULL, nombre, 0) para comprobar que
 * un nombre de simbolo se puede convertir.
 */

size_t mbstowcs(wchar_t *dst, const char *src, size_t n) {
    size_t i = 0;

    if (src == MIOS_NULL) {
        return (size_t)-1;
    }

    while (src[i] != '\0') {
        if (dst != MIOS_NULL) {
            if (i >= n) {
                return i;               /* no cabe mas: lo escrito es valido */
            }
            dst[i] = (wchar_t)(unsigned char)src[i];
        }
        i++;
    }

    if (dst != MIOS_NULL && i < n) {
        dst[i] = 0;
    }
    return i;
}

size_t wcstombs(char *dst, const wchar_t *src, size_t n) {
    size_t i = 0;

    if (src == MIOS_NULL) {
        return (size_t)-1;
    }

    while (src[i] != 0) {
        if ((unsigned long)src[i] > 255UL) {
            return (size_t)-1;          /* no es un byte: no representable */
        }
        if (dst != MIOS_NULL) {
            if (i >= n) {
                return i;
            }
            dst[i] = (char)src[i];
        }
        i++;
    }

    if (dst != MIOS_NULL && i < n) {
        dst[i] = '\0';
    }
    return i;
}
