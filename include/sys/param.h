#ifndef MIOS_SYS_PARAM_H
#define MIOS_SYS_PARAM_H

/*
 * MiOS - sys/param.h (cabecera publica).
 *
 * Nombres clasicos de BSD que mucho codigo espera (limites del sistema y
 * macros de redondeo).  glibc tambien la trae; aqui esta porque libctf
 * (la usa bfd/binutils) la incluye.
 *
 * No define MIN/MAX: cada proyecto suele traer los suyos y asi no chocan.
 */

#include <limits.h>
#include "types.h"

#ifndef MAXPATHLEN
#define MAXPATHLEN PATH_MAX
#endif

#ifndef MAXSYMLINKS
#define MAXSYMLINKS 20
#endif

#ifndef NBBY
#define NBBY CHAR_BIT
#endif

#ifndef NGROUPS
#define NGROUPS 32
#endif

/* howmany(x, y): cuantos grupos de tamano y caben en x. */
#ifndef howmany
#define howmany(x, y) (((x) + ((y) - 1)) / (y))
#endif

#ifndef roundup
#define roundup(x, y) ((((x) + ((y) - 1)) / (y)) * (y))
#endif

#ifndef powerof2
#define powerof2(x) ((((x) - 1) & (x)) == 0)
#endif

#endif /* MIOS_SYS_PARAM_H */
