#ifndef MIOS_FEATURES_H
#define MIOS_FEATURES_H

/*
 * MiOS - features.h (cabecera publica).
 *
 * En glibc, <features.h> decide que trozos de las cabeceras se exponen segun
 * _POSIX_C_SOURCE, _GNU_SOURCE, etc.  En MiOS se expone siempre todo, asi que
 * esta cabecera esta vacia a proposito: existe porque mucho codigo (binutils,
 * gcc, ...) la incluye esperandola.
 *
 * A proposito NO define __GLIBC__: hay codigo que, al verlo, da por hecho que
 * detras esta glibc (los "internals" de <sys/procfs.h>, plugin-api.h, ...).
 */

/* glibc la usa para consultar caracteristicas (_ISOC23, ISOC2X, ...).  MiOS no
 * tiene ninguna, asi que siempre contesta que no. */
#define __GLIBC_USE(x) 0

#endif /* MIOS_FEATURES_H */
