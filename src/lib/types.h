#ifndef MIOS_LIB_TYPES_H
#define MIOS_LIB_TYPES_H

/*
 * MiOS - tipos basicos.
 * No usamos los tipos de libc: el sistema corre sin ella.
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

#endif /* MIOS_LIB_TYPES_H */
