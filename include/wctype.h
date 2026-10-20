#ifndef MIOS_WCTYPE_H
#define MIOS_WCTYPE_H

/*
 * MiOS - wctype.h (cabecera publica).
 *
 * Clasificacion de caracteres anchos: MiOS no tiene (es ASCII/UTF-8 con
 * funciones de byte).  Solo se dan los tipos y el cambio de caja, porque hay
 * codigo que incluye esta cabecera (bfd/peXXigen.c usa towlower()).
 */

#ifndef MIOS_WINT_T_DEFINED
#define MIOS_WINT_T_DEFINED
typedef unsigned int wint_t;
#endif

typedef unsigned long wctype_t;

#ifndef WEOF
#define WEOF 0xffffffffu
#endif

/* Cambio de caja ASCII (la localizacion "C"): el resto se devuelve igual. */
wint_t towlower(wint_t c);
wint_t towupper(wint_t c);

#endif /* MIOS_WCTYPE_H */
