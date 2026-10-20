#ifndef MIOS_STDIO_H
#define MIOS_STDIO_H

/*
 * MiOS - stdio.h (cabecera publica).
 *
 * FILE y los ficheros los implementa libmios.so.6 (src/lib/stdio.c); stdin,
 * stdout y stderr son punteros de datos exportados por la libreria.
 *
 * Las funciones con numero variable de argumentos se declaran con va_list (de
 * <stdarg.h>) y no con __builtin_va_list: asi la cabecera tambien vale para
 * compiladores distintos de gcc (tcc, el que corre dentro de MiOS).
 */

#include <stddef.h>
#include <stdarg.h>
#include "mios_types.h"

#ifndef MIOS_SSIZE_T_DEFINED
#define MIOS_SSIZE_T_DEFINED
typedef long ssize_t;
#endif

typedef struct _mios_FILE FILE;

/*
 * Marcas de compatibilidad: hay software que "reconoce" que existe <stdio.h>
 * por el nombre de la guarda de glibc (_STDIO_H) en vez de por FILE -- GMP lo
 * hace, y si no la ve esconde todos sus prototipos con FILE ("gmp.h doesnt
 * recognise <stdio.h>, FILE prototypes will be unavailable").  Se define la de
 * glibc, que es el ABI al que imitamos.
 */
#ifndef _STDIO_H
#define _STDIO_H 1
#endif

#define EOF (-1)

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

FILE  *fopen(const char *path, const char *mode);
FILE  *fdopen(int fd, const char *mode);
FILE  *tmpfile(void);
int    fclose(FILE *f);
int    fflush(FILE *f);

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *f);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *f);

int    fgetc(FILE *f);
int    getc(FILE *f);
int    getchar(void);
int    ungetc(int c, FILE *f);
int    fputc(int c, FILE *f);
int    putc(int c, FILE *f);
int    putchar(int c);

char  *fgets(char *s, int size, FILE *f);
int    fputs(const char *s, FILE *f);
int    puts(const char *s);

int    feof(FILE *f);
int    ferror(FILE *f);
void   clearerr(FILE *f);
int    fileno(FILE *f);

int    fseek(FILE *f, long offset, int whence);
long   ftell(FILE *f);
void   rewind(FILE *f);
int    setvbuf(FILE *f, char *buf, int mode, size_t size);
void   setbuf(FILE *f, char *buf);

int    printf(const char *fmt, ...);
int    fprintf(FILE *f, const char *fmt, ...);
int    vprintf(const char *fmt, va_list ap);
int    vfprintf(FILE *f, const char *fmt, va_list ap);
int    sprintf(char *buf, const char *fmt, ...);
int    snprintf(char *buf, size_t size, const char *fmt, ...);
int    vsprintf(char *buf, const char *fmt, va_list ap);
int    vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);
int    scanf(const char *fmt, ...);
int    fscanf(FILE *f, const char *fmt, ...);
int    sscanf(const char *s, const char *fmt, ...);
int    vsscanf(const char *s, const char *fmt, va_list ap);
int    vfscanf(FILE *f, const char *fmt, va_list ap);

int    remove(const char *path);
void   perror(const char *s);

/* Salida de MiOS al margen de stdio (con espejo por el puerto serie). */
void cons_printf(const char *fmt, ...);
void cons_puts(const char *s);
void cons_putc(char c);
void cons_flush(void);

#endif /* MIOS_STDIO_H */
