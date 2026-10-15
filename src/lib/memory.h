#ifndef MIOS_LIB_MEMORY_H
#define MIOS_LIB_MEMORY_H

#include "lib/types.h"

void *mios_memset(void *dst, int c, mios_size_t n);
void *mios_memcpy(void *dst, const void *src, mios_size_t n);
void *mios_memmove(void *dst, const void *src, mios_size_t n);
int   mios_memcmp(const void *a, const void *b, mios_size_t n);

#endif /* MIOS_LIB_MEMORY_H */
