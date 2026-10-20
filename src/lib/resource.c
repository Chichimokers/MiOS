#include "lib/lib.h"
#include "lib/ret.h"
#include "kernel/syscall.h"
#include "../../include/mios.h"

/*
 * MiOS - limites y consumo de recursos (sys/resource.h) y consultas del
 * sistema (getpagesize/sysconf en unistd.h).
 *
 * Fase 7: binutils (gas/ld) las pide al arrancar y al informar de memoria.
 * Los valores de sysconf son los de MiOS: una sola CPU, paginas de 4 KiB y
 * la misma configuracion que ve un programa normal de Linux.
 */

long sysconf(int name) {
    switch (name) {
    case _SC_PAGESIZE:        return 4096;
    case _SC_CLK_TCK:         return 100;         /* el reloj del kernel va a 100 Hz */
    case _SC_OPEN_MAX:        return 1024;
    case _SC_ARG_MAX:         return 131072;
    case _SC_CHILD_MAX:       return 512;
    case _SC_NGROUPS_MAX:     return 32;
    case _SC_STREAM_MAX:      return 16;
    case _SC_TZNAME_MAX:      return 6;
    case _SC_NPROCESSORS_CONF:return 1;
    case _SC_NPROCESSORS_ONLN:return 1;
    case _SC_PHYS_PAGES:      return 262144;      /* 1 GiB de RAM en paginas */
    case _SC_AVPHYS_PAGES:    return 262144;
    case _SC_HOST_NAME_MAX:   return 64;
    case _SC_LINE_MAX:        return 2048;
    case _SC_JOB_CONTROL:
    case _SC_SAVED_IDS:
    case _SC_VERSION:         return 1;
    default:                  return -1;
    }
}

int getpagesize(void) {
    return 4096;
}

int getrusage(int who, struct rusage *usage) {
    if (usage == MIOS_NULL) {
        return -1;
    }
    /* La estructura es identica a la del kernel (sys/resource.h). */
    return (int)mios_ret(mios_getrusage(who, usage));
}

int getrlimit(int resource, struct rlimit *rlim) {
    if (rlim == MIOS_NULL) {
        return -1;
    }
    return (int)mios_ret(mios_getrlimit(resource, rlim));
}

int setrlimit(int resource, const struct rlimit *rlim) {
    if (rlim == MIOS_NULL) {
        return -1;
    }
    return (int)mios_ret(mios_setrlimit(resource, rlim));
}
