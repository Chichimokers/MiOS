#ifndef MIOS_SYS_TIME_H
#define MIOS_SYS_TIME_H

/*
 * MiOS - sys/time.h (cabecera publica).
 *
 * En MiOS, <time.h> ya declara struct timeval y gettimeofday(); esta cabecera
 * existe porque hay codigo (tcc, binutils, ...) que incluye <sys/time.h>.
 *
 * OJO: el #include de time.h va con "../time.h", no con "time.h".  Un include
 * entrecomillado se busca primero *relativo al fichero que lo escribe*, o sea
 * que desde aqui "time.h" seria include/sys/time.h: este mismo fichero.  La
 * guarda lo saltaria sin error y timeval nunca quedaria declarado.
 */

#include "types.h"
#include "../time.h"

struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};

#endif /* MIOS_SYS_TIME_H */
