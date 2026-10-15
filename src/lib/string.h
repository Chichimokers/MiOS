#ifndef MIOS_LIB_STRING_H
#define MIOS_LIB_STRING_H

#include "lib/types.h"

mios_size_t mios_strlen(const char *s);
int   mios_strcmp(const char *a, const char *b);
int   mios_strncmp(const char *a, const char *b, mios_size_t n);
char *mios_strcpy(char *dst, const char *src);
char *mios_strncpy(char *dst, const char *src, mios_size_t n);
char *mios_strchr(const char *s, int c);
int   mios_split_words(char *s, char **tokens, int max);

#endif /* MIOS_LIB_STRING_H */
