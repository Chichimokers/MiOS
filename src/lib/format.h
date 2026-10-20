#ifndef MIOS_LIB_FORMAT_H
#define MIOS_LIB_FORMAT_H

#include "lib/types.h"

/*
 * MiOS - motor de formato.
 *
 * Todo el formateo (%d %s %x %f...) pasa por aqui: la consola, printf,
 * snprintf, etc. solo aportan la funcion que recibe cada caracter.
 */

typedef void (*mios_put_fn)(void *ctx, char c);

int mios_vformat(mios_put_fn put, void *ctx, const char *fmt,
                 __builtin_va_list ap);

int mios_vsnprintf(char *buf, mios_size_t size, const char *fmt,
                   __builtin_va_list ap);

#endif /* MIOS_LIB_FORMAT_H */
