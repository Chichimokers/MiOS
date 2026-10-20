#include "lib/lib.h"
#include "lib/ret.h"
#include "kernel/syscall.h"
#include "../../include/mios.h"
#include "../../include/fenv.h"

/*
 * MiOS - fenv.h (Fase 3).
 *
 * Entorno de coma flotante sobre x87 + MXCSR.  Los modos de redondeo y las
 * excepciones son los mismos bits en ambos sitios, asi que se mantienen
 * sincronizados: el programa puede usar `double` (SSE) o `long double` (x87)
 * y el modo elegido vale para los dos.
 *
 * `fnstenv` enmascara las excepciones al guardar: por eso, tras cada uso, se
 * recarga la palabra de control con `fldcw` para no alterar el estado.
 */

struct x87env {
    unsigned short cw;
    unsigned short _a;
    unsigned short sw;
    unsigned short _b;
    unsigned short tw;
    unsigned short _c;
    unsigned int   ip;
    unsigned int   csop;
    unsigned int   dp;
    unsigned int   ds;
};

#define MXCSR_RC_MASK  0x6000u  /* bits 13-14: modo de redondeo */
#define MXCSR_EXC_MASK 0x1f80u  /* bits 7-12: excepciones enmascaradas */

static void read_mxcsr(unsigned int *v) {
    __asm__ volatile("stmxcsr %0" : "=m"(*v));
}

static void write_mxcsr(unsigned int v) {
    __asm__ volatile("ldmxcsr %0" : : "m"(v));
}

/* Guarda el entorno x87 sin alterar el estado del procesador. */
static void save_x87(struct x87env *e) {
    __asm__ volatile("fnstenv %0" : "=m"(*e));
    __asm__ volatile("fldcw %0" : : "m"(e->cw));
}

static void load_x87(const struct x87env *e) {
    __asm__ volatile("fldenv %0" : : "m"(*e));
}

int fegetround(void) {
    unsigned short cw;

    __asm__ volatile("fnstcw %0" : "=m"(cw));
    return cw & 0x0C00;
}

int fesetround(int rounding_mode) {
    unsigned short cw;
    unsigned int mx;

    if ((rounding_mode & ~0x0C00) != 0) {
        return -1;
    }
    __asm__ volatile("fnstcw %0" : "=m"(cw));
    cw = (unsigned short)((cw & ~(unsigned short)0x0C00) | rounding_mode);
    __asm__ volatile("fldcw %0" : : "m"(cw));

    read_mxcsr(&mx);
    mx = (mx & ~MXCSR_RC_MASK) | ((unsigned int)rounding_mode << 3);
    write_mxcsr(mx);
    return 0;
}

int fegetenv(fenv_t *envp) {
    struct x87env e;

    save_x87(&e);
    envp->cw = e.cw;
    envp->sw = e.sw;
    read_mxcsr(&envp->mxcsr);
    return 0;
}

int fesetenv(const fenv_t *envp) {
    struct x87env e;
    unsigned int mx;

    if (envp == FE_DFL_ENV) {
        e.cw = 0x037f;
        e.sw = 0;
        mx = 0x1f80;
    } else {
        e.cw = envp->cw;
        e.sw = envp->sw;
        mx = envp->mxcsr;
    }
    e.tw = 0xffff;
    e.ip = 0;
    e.csop = 0;
    e.dp = 0;
    e.ds = 0;
    load_x87(&e);
    write_mxcsr(mx);
    return 0;
}

int feclearexcept(int excepts) {
    struct x87env e;
    unsigned int mx;

    save_x87(&e);
    e.sw = (unsigned short)(e.sw & ~(unsigned short)excepts);
    load_x87(&e);

    read_mxcsr(&mx);
    mx &= ~((unsigned int)excepts);
    write_mxcsr(mx);
    return 0;
}

int fetestexcept(int excepts) {
    struct x87env e;
    unsigned int mx;

    save_x87(&e);
    read_mxcsr(&mx);
    return (int)(((unsigned int)e.sw | mx) & (unsigned int)excepts);
}

int feraiseexcept(int excepts) {
    struct x87env e;
    unsigned int mx;

    save_x87(&e);
    e.sw = (unsigned short)(e.sw | (unsigned short)excepts);
    load_x87(&e);

    read_mxcsr(&mx);
    mx |= (unsigned int)excepts;
    write_mxcsr(mx);
    return 0;
}

int fegetexceptflag(fexcept_t *flagp, int excepts) {
    *flagp = (fexcept_t)fetestexcept(excepts);
    return 0;
}

int fesetexceptflag(const fexcept_t *flagp, int excepts) {
    struct x87env e;
    unsigned int mx;

    save_x87(&e);
    e.sw = (unsigned short)((e.sw & ~(unsigned short)excepts) |
                            (*flagp & (unsigned short)excepts));
    load_x87(&e);

    read_mxcsr(&mx);
    mx = (mx & ~(unsigned int)excepts) |
         ((unsigned int)*flagp & (unsigned int)excepts);
    write_mxcsr(mx);
    return 0;
}

int feholdexcept(fenv_t *envp) {
    struct x87env e;
    unsigned int mx;

    fegetenv(envp);

    save_x87(&e);
    e.sw = 0;
    e.cw = (unsigned short)(e.cw | 0x003f); /* enmascarar todo */
    load_x87(&e);

    read_mxcsr(&mx);
    mx = (mx & ~0x003fu) | MXCSR_EXC_MASK;
    write_mxcsr(mx);
    return 0;
}

int feupdateenv(const fenv_t *envp) {
    int ex = fetestexcept(FE_ALL_EXCEPT);

    fesetenv(envp);
    feraiseexcept(ex);
    return 0;
}
