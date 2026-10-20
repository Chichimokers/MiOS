#ifndef MIOS_FENV_H
#define MIOS_FENV_H

/*
 * MiOS - fenv.h (cabecera publica).
 *
 * Implementado sobre la unidad x87 y MXCSR (SSE).  Como MiOS es dueno de su
 * libc, fenv_t es una estructura propia (no hace falta copiar el layout de
 * glibc): guarda la palabra de control y de estado x87 mas MXCSR.
 */

typedef struct {
    unsigned short cw;   /* palabra de control x87 */
    unsigned short sw;   /* palabra de estado x87  */
    unsigned int   mxcsr;
} fenv_t;

typedef unsigned short fexcept_t;

/* Excepciones (bits de la palabra de estado x87). */
#define FE_INVALID   0x01
#define FE_DENORMAL  0x02
#define FE_DIVBYZERO 0x04
#define FE_OVERFLOW  0x08
#define FE_UNDERFLOW 0x10
#define FE_INEXACT   0x20
#define FE_ALL_EXCEPT 0x3F

/* Modos de redondeo (bits 10-11 de la palabra de control). */
#define FE_TONEAREST  0x0000
#define FE_DOWNWARD   0x0400
#define FE_UPWARD     0x0800
#define FE_TOWARDZERO 0x0C00

#define FE_DFL_ENV ((const fenv_t *)-1)

int  feclearexcept(int excepts);
int  fetestexcept(int excepts);
int  feraiseexcept(int excepts);
int  fegetexceptflag(fexcept_t *flagp, int excepts);
int  fesetexceptflag(const fexcept_t *flagp, int excepts);
int  fegetround(void);
int  fesetround(int rounding_mode);
int  fegetenv(fenv_t *envp);
int  feholdexcept(fenv_t *envp);
int  fesetenv(const fenv_t *envp);
int  feupdateenv(const fenv_t *envp);

#endif /* MIOS_FENV_H */
