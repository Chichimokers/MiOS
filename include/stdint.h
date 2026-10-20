#ifndef MIOS_STDINT_H
#define MIOS_STDINT_H

/*
 * MiOS - stdint.h (cabecera publica).
 *
 * Tipos de ancho fijo.  En x86_64 con el modelo LP64: char 8, short 16,
 * int 32 y long 64 bits.  Las guardas son las mismas que usan las demas
 * cabeceras publicas (para poder incluirlas en cualquier orden).
 */

#ifndef MIOS_INT_TYPES_DEFINED
#define MIOS_INT_TYPES_DEFINED

typedef signed char        int8_t;
typedef short              int16_t;
typedef int                int32_t;
typedef long               int64_t;
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long      uint64_t;

typedef signed char        int_least8_t;
typedef short              int_least16_t;
typedef int                int_least32_t;
typedef long               int_least64_t;
typedef unsigned char      uint_least8_t;
typedef unsigned short     uint_least16_t;
typedef unsigned int       uint_least32_t;
typedef unsigned long      uint_least64_t;

typedef signed char        int_fast8_t;
typedef long               int_fast16_t;
typedef long               int_fast32_t;
typedef long               int_fast64_t;
typedef unsigned char      uint_fast8_t;
typedef unsigned long      uint_fast16_t;
typedef unsigned long      uint_fast32_t;
typedef unsigned long      uint_fast64_t;

typedef long               intptr_t;
typedef unsigned long      uintptr_t;
typedef long               intmax_t;
typedef unsigned long      uintmax_t;

#endif /* MIOS_INT_TYPES_DEFINED */

#define INT8_MIN    (-128)
#define INT8_MAX    127
#define UINT8_MAX   255u
#define INT16_MIN   (-32768)
#define INT16_MAX   32767
#define UINT16_MAX  65535u
#define INT32_MIN   (-2147483647 - 1)
#define INT32_MAX   2147483647
#define UINT32_MAX  4294967295u
#define INT64_MIN   (-9223372036854775807L - 1)
#define INT64_MAX   9223372036854775807L
#define UINT64_MAX  18446744073709551615UL

#define INTPTR_MIN  INT64_MIN
#define INTPTR_MAX  INT64_MAX
#define UINTPTR_MAX UINT64_MAX
#define INTMAX_MIN  INT64_MIN
#define INTMAX_MAX  INT64_MAX
#define UINTMAX_MAX UINT64_MAX
#define SIZE_MAX    UINT64_MAX

#define INT8_C(v)   (v)
#define INT16_C(v)  (v)
#define INT32_C(v)  (v)
#define INT64_C(v)  (v##L)
#define UINT8_C(v)  (v)
#define UINT16_C(v) (v)
#define UINT32_C(v) (v##u)
#define UINT64_C(v) (v##UL)

#endif /* MIOS_STDINT_H */
