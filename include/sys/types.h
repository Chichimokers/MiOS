#ifndef MIOS_SYS_TYPES_H
#define MIOS_SYS_TYPES_H

/*
 * MiOS - sys/types.h (cabecera publica).
 *
 * Todos los typedefs llevan guarda porque varias cabeceras publicas los
 * necesitan y pueden incluirse en cualquier orden.
 */

#include <stddef.h>

#ifndef MIOS_SSIZE_T_DEFINED
#define MIOS_SSIZE_T_DEFINED
typedef long ssize_t;
#endif

#ifndef MIOS_DEV_T_DEFINED
#define MIOS_DEV_T_DEFINED
typedef unsigned long dev_t;
#endif

#ifndef MIOS_INO_T_DEFINED
#define MIOS_INO_T_DEFINED
typedef unsigned long ino_t;
#endif

#ifndef MIOS_NLINK_T_DEFINED
#define MIOS_NLINK_T_DEFINED
typedef unsigned long nlink_t;
#endif

#ifndef MIOS_MODE_T_DEFINED
#define MIOS_MODE_T_DEFINED
typedef unsigned int mode_t;
#endif

#ifndef MIOS_UID_T_DEFINED
#define MIOS_UID_T_DEFINED
typedef unsigned int uid_t;
#endif

#ifndef MIOS_GID_T_DEFINED
#define MIOS_GID_T_DEFINED
typedef unsigned int gid_t;
#endif

#ifndef MIOS_OFF_T_DEFINED
#define MIOS_OFF_T_DEFINED
typedef long off_t;
#endif

#ifndef MIOS_PID_T_DEFINED
#define MIOS_PID_T_DEFINED
typedef int pid_t;
#endif

#ifndef MIOS_TIME_T_DEFINED
#define MIOS_TIME_T_DEFINED
typedef long time_t;
#endif

#ifndef MIOS_CLOCK_T_DEFINED
#define MIOS_CLOCK_T_DEFINED
typedef long clock_t;
#endif

#ifndef MIOS_USECONDS_T_DEFINED
#define MIOS_USECONDS_T_DEFINED
typedef unsigned int useconds_t;
#endif

#ifndef MIOS_SUSECONDS_T_DEFINED
#define MIOS_SUSECONDS_T_DEFINED
typedef long suseconds_t;
#endif

/* Los demas tipos "de sistema" que pide el software portado (glibc x86_64). */
#ifndef MIOS_ID_T_DEFINED
#define MIOS_ID_T_DEFINED
typedef unsigned int id_t;
#endif

#ifndef MIOS_KEY_T_DEFINED
#define MIOS_KEY_T_DEFINED
typedef int key_t;
#endif

#ifndef MIOS_CADDR_T_DEFINED
#define MIOS_CADDR_T_DEFINED
typedef char *caddr_t;
#endif

#ifndef MIOS_DADDR_T_DEFINED
#define MIOS_DADDR_T_DEFINED
typedef int daddr_t;
#endif

#ifndef MIOS_BLKCNT_T_DEFINED
#define MIOS_BLKCNT_T_DEFINED
typedef long blkcnt_t;
#endif

#ifndef MIOS_BLKSIZE_T_DEFINED
#define MIOS_BLKSIZE_T_DEFINED
typedef long blksize_t;
#endif

#ifndef MIOS_FSBLKCNT_T_DEFINED
#define MIOS_FSBLKCNT_T_DEFINED
typedef unsigned long fsblkcnt_t;
#endif

#ifndef MIOS_FSFILCNT_T_DEFINED
#define MIOS_FSFILCNT_T_DEFINED
typedef unsigned long fsfilcnt_t;
#endif

#ifndef MIOS_TIMER_T_DEFINED
#define MIOS_TIMER_T_DEFINED
typedef void *timer_t;
#endif

#endif /* MIOS_SYS_TYPES_H */
