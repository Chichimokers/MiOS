#ifndef MIOS_UTIME_H
#define MIOS_UTIME_H

/*
 * MiOS - utime.h (cabecera publica).
 *
 * Cambiar las fechas de acceso/modificacion de un fichero.  Fase 7: lo usa
 * binutils al copiar ficheros (binutils/rename.c copia tambien las fechas).
 */

#include "sys/types.h"
#include "time.h"

#ifndef MIOS_DEF_UTIMBUF
#define MIOS_DEF_UTIMBUF
struct utimbuf {
    time_t actime;
    time_t modtime;
};
#endif

/* times == NULL significa "ahora". */
int utime(const char *path, const struct utimbuf *times);

#endif /* MIOS_UTIME_H */
