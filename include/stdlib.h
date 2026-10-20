#ifndef MIOS_STDLIB_H
#define MIOS_STDLIB_H

/*
 * MiOS - stdlib.h (cabecera publica).
 */

#include <stddef.h>
#include "mios_types.h"

#ifndef MIOS_SSIZE_T_DEFINED
#define MIOS_SSIZE_T_DEFINED
typedef long ssize_t;
#endif

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

/* Memoria dinamica (src/lib/malloc.c) */
void *malloc(size_t n);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *p, size_t n);
void  free(void *p);

/* Procesos */
void exit(int status) __attribute__((noreturn));
void _Exit(int status) __attribute__((noreturn));
int  atexit(void (*fn)(void));
void abort(void) __attribute__((noreturn));

/* Entorno (el puntero lo mantiene libmios; crt1.o lo rellena al arrancar) */
extern char **environ;
char *getenv(const char *name);
int   setenv(const char *name, const char *value, int overwrite);
int   unsetenv(const char *name);
int   putenv(char *string);

/* Conversion */
int            atoi(const char *s);
long           atol(const char *s);
long long      atoll(const char *s);
long           strtol(const char *s, char **endp, int base);
long long      strtoll(const char *s, char **endp, int base);
unsigned long  strtoul(const char *s, char **endp, int base);
unsigned long long strtoull(const char *s, char **endp, int base);
double         strtod(const char *s, char **endp);
float          strtof(const char *s, char **endp);
long double    strtold(const char *s, char **endp);
double         atof(const char *s);

/* Ordenacion y busqueda */
void  qsort(void *base, size_t nmemb, size_t size,
            int (*cmp)(const void *, const void *));
void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
              int (*cmp)(const void *, const void *));

/* Aritmetica */
int          abs(int v);
long         labs(long v);
long long    llabs(long long v);

typedef struct { int quot, rem; } div_t;
typedef struct { long quot, rem; } ldiv_t;
typedef struct { long long quot, rem; } lldiv_t;

div_t   div(int num, int den);
ldiv_t  ldiv(long num, long den);
lldiv_t lldiv(long long num, long long den);

/* Aleatorios */
int  rand(void);
void srand(unsigned int seed);

/* Ficheros temporales (Fase 7: los usa libiberty, o sea binutils) */
#ifndef TMP_MAX
#define TMP_MAX 238328
#endif
int   mkstemp(char *pattern);
int   mkstemps(char *pattern, int suffix_len);
char *mkdtemp(char *pattern);
char *mktemp(char *pattern);

/* Rutas canonicas (Fase 7: las usa libiberty, o sea binutils).
   canonicalize_file_name() es la variante de GNU (equivale a realpath(p, 0)). */
char *realpath(const char *path, char *resolved);
char *canonicalize_file_name(const char *path);

#endif /* MIOS_STDLIB_H */
