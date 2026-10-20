#ifndef MIOS_SYS_RESOURCE_H
#define MIOS_SYS_RESOURCE_H

/*
 * MiOS - sys/resource.h (cabecera publica).
 *
 * Limites de recursos y consumo (getrusage/getrlimit/setrlimit).  Las
 * estructuras son las de Linux x86_64, que es lo que espera el kernel.
 */

#include "types.h"
#include "../time.h"

#define RUSAGE_SELF     0
#define RUSAGE_CHILDREN (-1)
#define RUSAGE_THREAD   1

#define RLIMIT_CPU     0
#define RLIMIT_FSIZE   1
#define RLIMIT_DATA    2
#define RLIMIT_STACK   3
#define RLIMIT_CORE    4
#define RLIMIT_RSS     5
#define RLIMIT_NPROC   6
#define RLIMIT_NOFILE  7
#define RLIMIT_MEMLOCK 8
#define RLIMIT_AS      9

#ifndef MIOS_RLIM_T_DEFINED
#define MIOS_RLIM_T_DEFINED
/* Tipo de los limites (glibc: __rlim_t -> unsigned long en x86_64). */
typedef unsigned long rlim_t;
#endif

#define RLIM_INFINITY (~0UL)
#define RLIM_SAVED_MAX RLIM_INFINITY
#define RLIM_SAVED_CUR RLIM_INFINITY

#ifndef MIOS_DEF_RLIMIT
#define MIOS_DEF_RLIMIT
struct rlimit {
    unsigned long rlim_cur;
    unsigned long rlim_max;
};
#endif

#ifndef MIOS_DEF_RUSAGE
#define MIOS_DEF_RUSAGE
struct rusage {
    struct timeval ru_utime;
    struct timeval ru_stime;
    long ru_maxrss;
    long ru_ixrss;
    long ru_idrss;
    long ru_isrss;
    long ru_minflt;
    long ru_majflt;
    long ru_nswap;
    long ru_inblock;
    long ru_oublock;
    long ru_msgsnd;
    long ru_msgrcv;
    long ru_nsignals;
    long ru_nvcsw;
    long ru_nivcsw;
};
#endif

int getrusage(int who, struct rusage *usage);
int getrlimit(int resource, struct rlimit *rlim);
int setrlimit(int resource, const struct rlimit *rlim);

#endif /* MIOS_SYS_RESOURCE_H */
