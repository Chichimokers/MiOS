#ifndef MIOS_CTYPE_H
#define MIOS_CTYPE_H

/*
 * MiOS - ctype.h (cabecera publica).
 * Funciones normales (no macros), que es lo que necesita el codigo portado.
 */

int isspace(int c);
int isdigit(int c);
int isxdigit(int c);
int isupper(int c);
int islower(int c);
int isalpha(int c);
int isalnum(int c);
int isprint(int c);
int isgraph(int c);
int ispunct(int c);
int iscntrl(int c);
int isblank(int c);
int isascii(int c);
int toascii(int c);
int toupper(int c);
int tolower(int c);

#endif /* MIOS_CTYPE_H */
