#ifndef MIOS_FS_H
#define MIOS_FS_H

#include "lib/types.h"

/*
 * MiOS - operaciones de archivos de alto nivel.
 * Todas devuelven 0 o un negativo estilo syscall (-errno).
 */

long file_read(const char *path, char *buf, mios_size_t max);
int  file_copy(const char *src, const char *dst);
void file_cat(const char *path);

#endif /* MIOS_FS_H */
