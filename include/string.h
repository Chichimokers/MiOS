#ifndef MIOS_STRING_H
#define MIOS_STRING_H

/*
 * MiOS - string.h (cabecera publica).
 */

#include <stddef.h>

void  *memcpy(void *dst, const void *src, size_t n);
void  *memmove(void *dst, const void *src, size_t n);
void  *memset(void *dst, int c, size_t n);
int    memcmp(const void *a, const void *b, size_t n);
void  *memchr(const void *s, int c, size_t n);

size_t strlen(const char *s);
int    strcmp(const char *a, const char *b);
int    strncmp(const char *a, const char *b, size_t n);
char  *strcpy(char *dst, const char *src);
char  *strncpy(char *dst, const char *src, size_t n);
char  *strcat(char *dst, const char *src);
char  *strncat(char *dst, const char *src, size_t n);
char  *strchr(const char *s, int c);
char  *strrchr(const char *s, int c);
char  *strstr(const char *haystack, const char *needle);
char  *strtok(char *s, const char *delim);
char  *strtok_r(char *s, const char *delim, char **saveptr);
char  *strdup(const char *s);
char  *strndup(const char *s, size_t n);
char  *stpcpy(char *dst, const char *src);
size_t strspn(const char *s, const char *accept);
size_t strcspn(const char *s, const char *reject);
char  *strpbrk(const char *s, const char *accept);
int    strcasecmp(const char *a, const char *b);
int    strncasecmp(const char *a, const char *b, size_t n);
size_t strnlen(const char *s, size_t maxlen);
char  *strchrnul(const char *s, int c);
void  *mempcpy(void *dst, const void *src, size_t n);

/* Comparacion segun la localizacion: en MiOS ("C") son strcmp/strcpy. */
int    strcoll(const char *a, const char *b);
size_t strxfrm(char *dst, const char *src, size_t n);

#endif /* MIOS_STRING_H */
