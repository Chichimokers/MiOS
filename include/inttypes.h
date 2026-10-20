#ifndef MIOS_INTTYPES_H
#define MIOS_INTTYPES_H

/*
 * MiOS - inttypes.h (cabecera publica).
 *
 * Los formatos de printf/scanf para los tipos de <stdint.h>.  En x86_64
 * (LP64): int64 es 'long' ("l...", como en glibc) y los tipos de 8, 16 y 32
 * bits se pasan como int (sin modificador).  Fase 7: binutils usa SCNu64 y
 * compania.
 */

#include <stdint.h>

/* Maximos */
#ifndef MIOS_INTMAX_T_DEFINED
#define MIOS_INTMAX_T_DEFINED
typedef long          intmax_t;
typedef unsigned long uintmax_t;
#endif

#define PRId8   "d"
#define PRId16  "d"
#define PRId32  "d"
#define PRId64  "ld"
#define PRIi8   "i"
#define PRIi16  "i"
#define PRIi32  "i"
#define PRIi64  "li"
#define PRIu8   "u"
#define PRIu16  "u"
#define PRIu32  "u"
#define PRIu64  "lu"
#define PRIo8   "o"
#define PRIo16  "o"
#define PRIo32  "o"
#define PRIo64  "lo"
#define PRIx8   "x"
#define PRIx16  "x"
#define PRIx32  "x"
#define PRIx64  "lx"
#define PRIX8   "X"
#define PRIX16  "X"
#define PRIX32  "X"
#define PRIX64  "lX"

#define PRIdLEAST8   "d"
#define PRIdLEAST16  "d"
#define PRIdLEAST32  "d"
#define PRIdLEAST64  "ld"
#define PRIiLEAST8   "i"
#define PRIiLEAST16  "i"
#define PRIiLEAST32  "i"
#define PRIiLEAST64  "li"
#define PRIuLEAST8   "u"
#define PRIuLEAST16  "u"
#define PRIuLEAST32  "u"
#define PRIuLEAST64  "lu"
#define PRIoLEAST8   "o"
#define PRIoLEAST16  "o"
#define PRIoLEAST32  "o"
#define PRIoLEAST64  "lo"
#define PRIxLEAST8   "x"
#define PRIxLEAST16  "x"
#define PRIxLEAST32  "x"
#define PRIxLEAST64  "lx"
#define PRIXLEAST8   "X"
#define PRIXLEAST16  "X"
#define PRIXLEAST32  "X"
#define PRIXLEAST64  "lX"

#define PRIdFAST8   "d"
#define PRIdFAST16  "d"
#define PRIdFAST32  "d"
#define PRIdFAST64  "ld"
#define PRIiFAST8   "i"
#define PRIiFAST16  "i"
#define PRIiFAST32  "i"
#define PRIiFAST64  "li"
#define PRIuFAST8   "u"
#define PRIuFAST16  "u"
#define PRIuFAST32  "u"
#define PRIuFAST64  "lu"
#define PRIoFAST8   "o"
#define PRIoFAST16  "o"
#define PRIoFAST32  "o"
#define PRIoFAST64  "lo"
#define PRIxFAST8   "x"
#define PRIxFAST16  "x"
#define PRIxFAST32  "x"
#define PRIxFAST64  "lx"
#define PRIXFAST8   "X"
#define PRIXFAST16  "X"
#define PRIXFAST32  "X"
#define PRIXFAST64  "lX"

#define PRIdMAX  "ld"
#define PRIiMAX  "li"
#define PRIuMAX  "lu"
#define PRIoMAX  "lo"
#define PRIxMAX  "lx"
#define PRIXMAX  "lX"
#define PRIdPTR  "ld"
#define PRIiPTR  "li"
#define PRIuPTR  "lu"
#define PRIoPTR  "lo"
#define PRIxPTR  "lx"
#define PRIXPTR  "lX"

#define SCNd8   "hhd"
#define SCNd16  "hd"
#define SCNd32  "d"
#define SCNd64  "ld"
#define SCNi8   "hhi"
#define SCNi16  "hi"
#define SCNi32  "i"
#define SCNi64  "li"
#define SCNu8   "hhu"
#define SCNu16  "hu"
#define SCNu32  "u"
#define SCNu64  "lu"
#define SCNo8   "hho"
#define SCNo16  "ho"
#define SCNo32  "o"
#define SCNo64  "lo"
#define SCNx8   "hhx"
#define SCNx16  "hx"
#define SCNx32  "x"
#define SCNx64  "lx"

#define SCNdLEAST8   "hhd"
#define SCNdLEAST16  "hd"
#define SCNdLEAST32  "d"
#define SCNdLEAST64  "ld"
#define SCNiLEAST8   "hhi"
#define SCNiLEAST16  "hi"
#define SCNiLEAST32  "i"
#define SCNiLEAST64  "li"
#define SCNuLEAST8   "hhu"
#define SCNuLEAST16  "hu"
#define SCNuLEAST32  "u"
#define SCNuLEAST64  "lu"
#define SCNoLEAST8   "hho"
#define SCNoLEAST16  "ho"
#define SCNoLEAST32  "o"
#define SCNoLEAST64  "lo"
#define SCNxLEAST8   "hhx"
#define SCNxLEAST16  "hx"
#define SCNxLEAST32  "x"
#define SCNxLEAST64  "lx"

#define SCNdFAST8   "hhd"
#define SCNdFAST16  "hd"
#define SCNdFAST32  "d"
#define SCNdFAST64  "ld"
#define SCNiFAST8   "hhi"
#define SCNiFAST16  "hi"
#define SCNiFAST32  "i"
#define SCNiFAST64  "li"
#define SCNuFAST8   "hhu"
#define SCNuFAST16  "hu"
#define SCNuFAST32  "u"
#define SCNuFAST64  "lu"
#define SCNoFAST8   "hho"
#define SCNoFAST16  "ho"
#define SCNoFAST32  "o"
#define SCNoFAST64  "lo"
#define SCNxFAST8   "hhx"
#define SCNxFAST16  "hx"
#define SCNxFAST32  "x"
#define SCNxFAST64  "lx"

#define SCNdMAX  "ld"
#define SCNiMAX  "li"
#define SCNuMAX  "lu"
#define SCNoMAX  "lo"
#define SCNxMAX  "lx"
#define SCNdPTR  "ld"
#define SCNiPTR  "li"
#define SCNuPTR  "lu"
#define SCNoPTR  "lo"
#define SCNxPTR  "lx"

/* Conversion de cadenas a los tipos maximos */
intmax_t  strtoimax(const char *nptr, char **endptr, int base);
uintmax_t strtoumax(const char *nptr, char **endptr, int base);
intmax_t  imaxabs(intmax_t v);

typedef struct { intmax_t quot, rem; } imaxdiv_t;
imaxdiv_t imaxdiv(intmax_t num, intmax_t den);

#endif /* MIOS_INTTYPES_H */
