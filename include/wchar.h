#ifndef MIOS_WCHAR_H
#define MIOS_WCHAR_H

/*
 * MiOS - wchar.h (cabecera publica).
 *
 * MiOS trabaja con bytes (ASCII y UTF-8 tal cual): no hay locales ni juegos de
 * caracteres alternativos, asi que un caracter ancho es un byte (0..255) y la
 * conversion es la identidad.  Hay codigo que la incluye sin usar nada de ella
 * (por ejemplo bfd/peXXigen.c, el backend de PE de binutils).
 *
 * A proposito NO declara funciones que MiOS no tiene: declararlas solo
 * cambiaria el error de compilacion por uno de enlazado.
 */

#include <stddef.h>

/* wchar_t lo define el compilador en <stddef.h> (C99). */

#ifndef MIOS_WINT_T_DEFINED
#define MIOS_WINT_T_DEFINED
typedef unsigned int wint_t;
#endif

#ifndef WEOF
#define WEOF 0xffffffffu
#endif

/* Conversion multibyte <-> ancho (identidad byte a byte, sin locales). */
size_t mbstowcs(wchar_t *dst, const char *src, size_t n);
size_t wcstombs(char *dst, const wchar_t *src, size_t n);

#endif /* MIOS_WCHAR_H */
