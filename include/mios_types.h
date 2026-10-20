#ifndef MIOS_TYPES_H
#define MIOS_TYPES_H

/*
 * MiOS - tipos basicos (cabecera publica).
 *
 * Es la version "de cara al programador" de src/lib/types.h: MiOS no usa libc,
 * asi que sus tipos viven aqui. Se instala en <sysroot>/usr/include.
 */

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef unsigned long  u64;

typedef signed char  s8;
typedef signed short s16;
typedef signed int   s32;
typedef signed long  s64;

typedef unsigned long mios_size_t;
typedef long          mios_ssize_t;

#define MIOS_NULL ((void *)0)

#endif /* MIOS_TYPES_H */
