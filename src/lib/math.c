#include "lib/lib.h"
#include "lib/ret.h"
#include "kernel/syscall.h"
#include "../../include/mios.h"
#include "../../include/math.h"

/*
 * MiOS - math.h: biblioteca matematica propia (Fase 3).
 *
 * Sin libm del sistema.  Estrategia:
 *   - Lo exacto (fabs, copysign, frexp, ldexp, modf, floor/ceil/trunc/rint,
 *     nextafter, escalado) se hace con manipulacion de bits o con la unidad
 *     x87 (frndint, con el modo de redondeo que corresponda).
 *   - Lo trascendente usa reduccion de argumento + polinomios minimax tipo
 *     fdlibm (sin/cos/atan/asin/acos) o series reducidas (exp/log/pow).
 *   - Las variantes `float` delegan en la version `double` (exacto salvo
 *     redondeo final).  Las variantes `long double` usan x87 alli donde es
 *     exacto y barato (sqrtl, floorl, fmodl, frexpl...) y delegan en `double`
 *     en las trascendentes: precision "suficiente", no perfecta (documentado).
 */

#define DNAN (__builtin_nan(""))

typedef union { double d; u64 u; } dshape;
typedef union { float  f; u32 u; } fshape;
typedef union { long double ld; struct { u64 m; u16 e; } p; } lshape;

#define DBITS_MANT 0x000fffffffffffffULL
#define DBITS_SIGN 0x8000000000000000ULL

/* ------------------------------------------------------------------ */
/* Signo y valor absoluto (exactos)                                     */
/* ------------------------------------------------------------------ */

double fabs(double x) {
    dshape v;
    v.d = x;
    v.u &= ~DBITS_SIGN;
    return v.d;
}

float fabsf(float x) {
    fshape v;
    v.f = x;
    v.u &= 0x7fffffffu;
    return v.f;
}

long double fabsl(long double x) {
    lshape v;
    v.ld = x;
    v.p.e &= 0x7fffu;
    return v.ld;
}

double copysign(double x, double y) {
    dshape a, b;
    a.d = x;
    b.d = y;
    a.u = (a.u & ~DBITS_SIGN) | (b.u & DBITS_SIGN);
    return a.d;
}

float copysignf(float x, float y) {
    fshape a, b;
    a.f = x;
    b.f = y;
    a.u = (a.u & 0x7fffffffu) | (b.u & 0x80000000u);
    return a.f;
}

long double copysignl(long double x, long double y) {
    lshape a, b;
    a.ld = x;
    b.ld = y;
    a.p.e = (u16)((a.p.e & 0x7fff) | (b.p.e & 0x8000));
    return a.ld;
}

int __signbit(double x) {
    dshape v;
    v.d = x;
    return (int)(v.u >> 63);
}

int __signbitf(float x) {
    fshape v;
    v.f = x;
    return (int)(v.u >> 31);
}

int __signbitl(long double x) {
    lshape v;
    v.ld = x;
    return (v.p.e & 0x8000) ? 1 : 0;
}

int __fpclassify(double x) {
    dshape v;
    u64 e;
    v.d = x;
    e = (v.u >> 52) & 0x7ff;
    if (e == 0x7ff) {
        return (v.u & DBITS_MANT) ? FP_NAN : FP_INFINITE;
    }
    if (e == 0) {
        return (v.u & DBITS_MANT) ? FP_SUBNORMAL : FP_ZERO;
    }
    return FP_NORMAL;
}

int __fpclassifyf(float x) {
    fshape v;
    u32 e;
    v.f = x;
    e = (v.u >> 23) & 0xff;
    if (e == 0xff) {
        return (v.u & 0x007fffffu) ? FP_NAN : FP_INFINITE;
    }
    if (e == 0) {
        return (v.u & 0x007fffffu) ? FP_SUBNORMAL : FP_ZERO;
    }
    return FP_NORMAL;
}

int __fpclassifyl(long double x) {
    lshape v;
    u16 e;
    v.ld = x;
    e = v.p.e & 0x7fff;
    if (e == 0x7fff) {
        return (v.p.m != 0x8000000000000000ULL) ? FP_NAN : FP_INFINITE;
    }
    if (e == 0) {
        return (v.p.m == 0) ? FP_ZERO : FP_SUBNORMAL;
    }
    return FP_NORMAL;
}

/*
 * ------------------------------------------------------------------
 * Coma flotante "a prueba de compiladores": infinito, NaN y las
 * comprobaciones, como funciones.
 *
 * gcc resuelve isnan/isinf/... con builtins; tcc (el compilador que corre
 * dentro de MiOS) no los tiene y usa estas funciones (ver include/math.h).
 * Se construyen con los mismos trucos de bits que el resto del fichero, asi
 * que son exactas y no dependen del compilador.
 * ------------------------------------------------------------------
 */

double __mios_huge_val(void) {
    dshape v;
    v.u = 0x7ff0000000000000ULL;
    return v.d;
}

float __mios_huge_valf(void) {
    fshape v;
    v.u = 0x7f800000u;
    return v.f;
}

long double __mios_huge_vall(void) {
    return (long double)__mios_huge_val();
}

double __mios_nan(void) {
    dshape v;
    v.u = 0x7ff8000000000000ULL;
    return v.d;
}

int __mios_isnan(double x) {
    return __fpclassify(x) == FP_NAN;
}

int __mios_isinf(double x) {
    return __fpclassify(x) == FP_INFINITE;
}

int __mios_isfinite(double x) {
    int c = __fpclassify(x);
    return c != FP_NAN && c != FP_INFINITE;
}

int __mios_isnormal(double x) {
    return __fpclassify(x) == FP_NORMAL;
}

/* ------------------------------------------------------------------ */
/* Redondeo: la unidad x87 (exacto, sin asumir rangos)                  */
/* ------------------------------------------------------------------ */

/* rc: 0 = al mas cercano (par), 1 = hacia abajo, 2 = hacia arriba, 3 = a cero */
static double x87_round(double x, int rc) {
    unsigned short cw, old;
    double r;

    __asm__ volatile("fnstcw %0" : "=m"(cw));
    old = cw;
    cw = (unsigned short)((cw & ~(unsigned short)0x0C00) |
                          (unsigned short)((rc & 3) << 10));
    __asm__ volatile("fldcw %0" : : "m"(cw));
    __asm__ volatile("frndint" : "=t"(r) : "0"(x));
    __asm__ volatile("fldcw %0" : : "m"(old));
    return r;
}

static long double x87_roundl(long double x, int rc) {
    unsigned short cw, old;
    long double r;

    __asm__ volatile("fnstcw %0" : "=m"(cw));
    old = cw;
    cw = (unsigned short)((cw & ~(unsigned short)0x0C00) |
                          (unsigned short)((rc & 3) << 10));
    __asm__ volatile("fldcw %0" : : "m"(cw));
    __asm__ volatile("frndint" : "=t"(r) : "0"(x));
    __asm__ volatile("fldcw %0" : : "m"(old));
    return r;
}

double trunc(double x) { return x87_round(x, 3); }
double floor(double x) { return x87_round(x, 1); }
double ceil(double x) { return x87_round(x, 2); }

/* rint/nearbyint redondean segun el modo actual: no se toca la palabra de
 * control, solo se pide la operacion. */
double rint(double x) {
    double r;
    __asm__ volatile("frndint" : "=t"(r) : "0"(x));
    return r;
}

double nearbyint(double x) { return rint(x); }

double round(double x) {
    double t = trunc(x);
    double d = x - t;

    if (d >= 0.5) {
        t += 1.0;
    } else if (d <= -0.5) {
        t -= 1.0;
    }
    return t;
}

float floorf(float x) { return (float)floor((double)x); }
float ceilf(float x) { return (float)ceil((double)x); }
float truncf(float x) { return (float)trunc((double)x); }
float roundf(float x) { return (float)round((double)x); }
float rintf(float x) { return (float)rint((double)x); }
float nearbyintf(float x) { return (float)nearbyint((double)x); }

long double floorl(long double x) { return x87_roundl(x, 1); }
long double ceill(long double x) { return x87_roundl(x, 2); }
long double truncl(long double x) { return x87_roundl(x, 3); }

long double rintl(long double x) {
    long double r;
    __asm__ volatile("frndint" : "=t"(r) : "0"(x));
    return r;
}

long double nearbyintl(long double x) { return rintl(x); }

long double roundl(long double x) {
    long double t = truncl(x);
    long double d = x - t;

    if (d >= 0.5L) {
        t += 1.0L;
    } else if (d <= -0.5L) {
        t -= 1.0L;
    }
    return t;
}

long lrint(double x) { return (long)rint(x); }
long lrintf(float x) { return (long)rintf(x); }
long lrintl(long double x) { return (long)rintl(x); }
long long llrint(double x) { return (long long)rint(x); }
long long llrintf(float x) { return (long long)rintf(x); }
long long llrintl(long double x) { return (long long)rintl(x); }
long lround(double x) { return (long)round(x); }
long lroundf(float x) { return (long)roundf(x); }
long lroundl(long double x) { return (long)roundl(x); }
long long llround(double x) { return (long long)round(x); }
long long llroundf(float x) { return (long long)roundf(x); }
long long llroundl(long double x) { return (long long)roundl(x); }

/* ------------------------------------------------------------------ */
/* Exponente y escalado (exactos)                                       */
/* ------------------------------------------------------------------ */

double frexp(double x, int *e) {
    dshape v;
    int ex;

    v.d = x;
    ex = (int)((v.u >> 52) & 0x7ff);

    if (ex == 0) {
        if (x == 0.0) {
            *e = 0;
            return x;
        }
        v.d = x * 9007199254740992.0; /* 2^53 */
        ex = (int)((v.u >> 52) & 0x7ff);
        *e = ex - 1022 - 53;
        v.u = (v.u & DBITS_MANT) | (0x3feULL << 52);
        return v.d;
    }
    if (ex == 0x7ff) {
        *e = 0;
        return x;
    }
    *e = ex - 1022;
    v.u = (v.u & (DBITS_SIGN | DBITS_MANT)) | (0x3feULL << 52);
    return v.d;
}

float frexpf(float x, int *e) {
    if (x == 0.0f || !isfinite(x)) {
        *e = 0;
        return x;
    }
    return (float)frexp((double)x, e);
}

long double frexpl(long double x, int *e) {
    lshape v;
    int ex;

    v.ld = x;
    ex = v.p.e & 0x7fff;

    if (ex == 0 && v.p.m == 0) {
        *e = 0;
        return x;
    }
    if (ex == 0x7fff) {
        *e = 0;
        return x;
    }
    if (ex == 0) {
        v.ld = x * 0x1p64L;
        ex = v.p.e & 0x7fff;
        *e = ex - 16382 - 64;
    } else {
        *e = ex - 16382;
    }
    v.p.e = (u16)((v.p.e & 0x8000) | 16382);
    return v.ld;
}

double ldexp(double x, int n) {
    dshape v;

    if (x == 0.0 || !isfinite(x) || n == 0) {
        return x;
    }
    while (n > 1023) {
        x *= 0x1p1023;
        n -= 1023;
        if (!isfinite(x)) {
            return x;
        }
    }
    while (n < -1022) {
        x *= 0x1p-1022;
        n += 1022;
        if (x == 0.0) {
            return x;
        }
    }
    v.u = (u64)(unsigned)(n + 1023) << 52;
    return x * v.d;
}

float ldexpf(float x, int n) { return (float)ldexp((double)x, n); }

long double ldexpl(long double x, int n) {
    lshape v;

    if (x == 0.0L || !isfinite(x) || n == 0) {
        return x;
    }
    while (n > 16383) {
        x *= 0x1p16383L;
        n -= 16383;
        if (!isfinite(x)) {
            return x;
        }
    }
    while (n < -16382) {
        x *= 0x1p-16382L;
        n += 16382;
        if (x == 0.0L) {
            return x;
        }
    }
    v.ld = 0.0L;
    v.p.m = 0x8000000000000000ULL;
    v.p.e = (u16)(n + 16383);
    return x * v.ld;
}

double scalbn(double x, int n) { return ldexp(x, n); }
float scalbnf(float x, int n) { return ldexpf(x, n); }
long double scalbnl(long double x, int n) { return ldexpl(x, n); }
double scalbln(double x, long n) { return ldexp(x, (int)n); }
float scalblnf(float x, long n) { return ldexpf(x, (int)n); }
long double scalblnl(long double x, long n) { return ldexpl(x, (int)n); }

double modf(double x, double *iptr) {
    double t = trunc(x);

    *iptr = t;
    return x - t;
}

float modff(float x, float *iptr) {
    float t = truncf(x);

    *iptr = t;
    return x - t;
}

long double modfl(long double x, long double *iptr) {
    long double t = truncl(x);

    *iptr = t;
    return x - t;
}

double logb(double x) {
    int e;

    if (x == 0.0) {
        return -HUGE_VAL;
    }
    if (!isfinite(x)) {
        return fabs(x);
    }
    frexp(x, &e);
    return (double)(e - 1);
}

int ilogb(double x) {
    int e;

    if (x == 0.0) {
        return FP_ILOGB0;
    }
    if (isnan(x)) {
        return FP_ILOGBNAN;
    }
    if (isinf(x)) {
        return 2147483647;
    }
    frexp(x, &e);
    return e - 1;
}

float logbf(float x) { return (float)logb((double)x); }
long double logbl(long double x) {
    int e;

    if (x == 0.0L) {
        return -HUGE_VALL;
    }
    if (!isfinite(x)) {
        return fabsl(x);
    }
    frexpl(x, &e);
    return (long double)(e - 1);
}

int ilogbf(float x) { return ilogb((double)x); }

int ilogbl(long double x) {
    int e;

    if (x == 0.0L) {
        return FP_ILOGB0;
    }
    if (isnan(x)) {
        return FP_ILOGBNAN;
    }
    if (isinf(x)) {
        return 2147483647;
    }
    frexpl(x, &e);
    return e - 1;
}

/* ------------------------------------------------------------------ */
/* Raiz cuadrada (instruccion del procesador: exacta)                   */
/* ------------------------------------------------------------------ */

double sqrt(double x) {
    double r;
    __asm__("sqrtsd %1, %0" : "=x"(r) : "x"(x));
    return r;
}

float sqrtf(float x) {
    float r;
    __asm__("sqrtss %1, %0" : "=x"(r) : "x"(x));
    return r;
}

long double sqrtl(long double x) {
    long double r;
    __asm__("fsqrt" : "=t"(r) : "0"(x));
    return r;
}

/* ------------------------------------------------------------------ */
/* Resto (exacto por restas, que en el rango que se usa son exactas)    */
/* ------------------------------------------------------------------ */

double fmod(double x, double y) {
    double ax, ay, yd, r;
    int ex, ey, k;

    if (isnan(x) || isnan(y) || isinf(x) || y == 0.0) {
        return DNAN;
    }
    if (isinf(y)) {
        return x;
    }
    if (x == 0.0) {
        return x;
    }

    ax = fabs(x);
    ay = fabs(y);
    if (ax < ay) {
        return x;
    }

    frexp(ax, &ex);
    frexp(ay, &ey);
    k = ex - ey;
    if (k < 0) {
        k = 0;
    }
    yd = ldexp(ay, k);
    while (!isfinite(yd) && k > 0) {
        k--;
        yd = ldexp(ay, k);
    }

    r = ax;
    for (; k >= 0; k--) {
        if (r >= yd) {
            r -= yd;
        }
        yd *= 0.5;
    }
    if (r >= ay) {
        r -= ay;
    }
    return copysign(r, x);
}

float fmodf(float x, float y) { return (float)fmod((double)x, (double)y); }

long double fmodl(long double x, long double y) {
    long double ax, ay, yd, r;
    int ex, ey, k;

    if (isnan(x) || isnan(y) || isinf(x) || y == 0.0L) {
        return DNAN;
    }
    if (isinf(y)) {
        return x;
    }
    if (x == 0.0L) {
        return x;
    }
    ax = fabsl(x);
    ay = fabsl(y);
    if (ax < ay) {
        return x;
    }
    frexpl(ax, &ex);
    frexpl(ay, &ey);
    k = ex - ey;
    if (k < 0) {
        k = 0;
    }
    yd = ldexpl(ay, k);
    while (!isfinite(yd) && k > 0) {
        k--;
        yd = ldexpl(ay, k);
    }
    r = ax;
    for (; k >= 0; k--) {
        if (r >= yd) {
            r -= yd;
        }
        yd *= 0.5L;
    }
    if (r >= ay) {
        r -= ay;
    }
    return copysignl(r, x);
}

double remainder(double x, double y) {
    double r = fmod(x, y);
    double ay = fabs(y);

    if (isnan(r)) {
        return r;
    }
    if (fabs(r) * 2.0 > ay) {
        r -= copysign(ay, r);
    }
    return r;
}

float remainderf(float x, float y) {
    return (float)remainder((double)x, (double)y);
}

long double remainderl(long double x, long double y) {
    long double r = fmodl(x, y);
    long double ay = fabsl(y);

    if (isnan(r)) {
        return r;
    }
    if (fabsl(r) * 2.0L > ay) {
        r -= copysignl(ay, r);
    }
    return r;
}

double remquo(double x, double y, int *quo) {
    double q = (y != 0.0) ? rint(x / y) : 0.0;
    double r = x - q * y;

    if (quo != (int *)0) {
        long long n = (long long)q;
        *quo = (int)(n & 7);
    }
    if (isfinite(x) && isfinite(y) && y != 0.0) {
        r = remainder(x, y);
    }
    return r;
}

float remquof(float x, float y, int *quo) {
    return (float)remquo((double)x, (double)y, quo);
}

long double remquol(long double x, long double y, int *quo) {
    long double q = (y != 0.0L) ? rintl(x / y) : 0.0L;

    if (quo != (int *)0) {
        *quo = (int)((long long)q & 7);
    }
    return remainderl(x, y);
}

/* ------------------------------------------------------------------ */
/* Minimos, maximos, diferencias, copia multiple                        */
/* ------------------------------------------------------------------ */

double fmax(double x, double y) {
    if (isnan(x)) {
        return y;
    }
    if (isnan(y)) {
        return x;
    }
    if (x == 0.0 && y == 0.0) {
        return signbit(x) ? y : x;
    }
    return (x > y) ? x : y;
}

double fmin(double x, double y) {
    if (isnan(x)) {
        return y;
    }
    if (isnan(y)) {
        return x;
    }
    if (x == 0.0 && y == 0.0) {
        return signbit(x) ? x : y;
    }
    return (x < y) ? x : y;
}

double fdim(double x, double y) {
    if (isnan(x) || isnan(y)) {
        return DNAN;
    }
    return (x > y) ? (x - y) : 0.0;
}

float fmaxf(float x, float y) { return (float)fmax((double)x, (double)y); }
float fminf(float x, float y) { return (float)fmin((double)x, (double)y); }
float fdimf(float x, float y) { return (float)fdim((double)x, (double)y); }
long double fmaxl(long double x, long double y) {
    if (isnan(x)) return y;
    if (isnan(y)) return x;
    return (x > y) ? x : y;
}
long double fminl(long double x, long double y) {
    if (isnan(x)) return y;
    if (isnan(y)) return x;
    return (x < y) ? x : y;
}
long double fdiml(long double x, long double y) {
    if (isnan(x) || isnan(y)) return DNAN;
    return (x > y) ? (x - y) : 0.0L;
}

double fma(double x, double y, double z) {
    /* x87 (80 bits) da el producto y la suma con un solo redondeo final. */
    return (double)((long double)x * (long double)y + (long double)z);
}

float fmaf(float x, float y, float z) {
    return (float)((double)x * (double)y + (double)z);
}

long double fmal(long double x, long double y, long double z) {
    return x * y + z;
}

double nextafter(double x, double y) {
    dshape a;

    if (isnan(x) || isnan(y)) {
        return x + y;
    }
    if (x == y) {
        return y;
    }
    if (x == 0.0) {
        a.u = 1;
        if (signbit(y)) {
            a.u |= DBITS_SIGN;
        }
        return a.d;
    }
    a.d = x;
    if ((x < y) == (x > 0.0)) {
        a.u++;
    } else {
        a.u--;
    }
    return a.d;
}

float nextafterf(float x, float y) { return (float)nextafter((double)x, (double)y); }

long double nextafterl(long double x, long double y) {
    lshape a;

    if (isnan(x) || isnan(y)) {
        return x + y;
    }
    if (x == y) {
        return y;
    }
    if (x == 0.0L) {
        a.ld = 0.0L;
        a.p.m = 1;
        if (signbit(y)) {
            a.p.e = 0x8000;
        }
        return a.ld;
    }
    a.ld = x;
    if ((x < y) == (x > 0.0L)) {
        a.p.m++;
        if (a.p.m == 0) {
            a.p.m = 0x8000000000000000ULL;
            a.p.e++;
        }
    } else {
        if (a.p.m == 0x8000000000000000ULL) {
            a.p.m = 0xffffffffffffffffULL;
            a.p.e--;
        } else {
            a.p.m--;
        }
    }
    return a.ld;
}

double nexttoward(double x, long double y) { return nextafter(x, (double)y); }
float nexttowardf(float x, long double y) { return (float)nextafter((double)x, (double)y); }
long double nexttowardl(long double x, long double y) { return nextafterl(x, y); }

/* ------------------------------------------------------------------ */
/* Exponencial y logaritmos                                             */
/* ------------------------------------------------------------------ */

#define LN2_HI  6.93147180369123816490e-01
#define LN2_LO  1.90821492927058770002e-10
#define INV_LN2 1.44269504088896338700e+00

double exp(double x) {
    double k, hi, lo, r, p;
    int n;

    if (isnan(x)) {
        return x;
    }
    if (x > 709.782712893384) {
        errno = ERANGE;
        return HUGE_VAL;
    }
    if (x < -745.1332191019411) {
        errno = ERANGE;
        return 0.0;
    }

    k = rint(x * INV_LN2);
    n = (int)k;
    hi = x - k * LN2_HI;
    lo = k * LN2_LO;
    r = hi - lo;

    p = 1.0 + r * (1.0 + r * (1.0 / 2.0 + r * (1.0 / 6.0 + r * (1.0 / 24.0 +
        r * (1.0 / 120.0 + r * (1.0 / 720.0 + r * (1.0 / 5040.0 + r *
        (1.0 / 40320.0 + r * (1.0 / 362880.0 + r * (1.0 / 3628800.0 + r *
        (1.0 / 39916800.0 + r * (1.0 / 479001600.0 + r * (1.0 / 6227020800.0 +
        r * (1.0 / 87178291200.0))))))))))))));

    return ldexp(p, n);
}

double exp2(double x) {
    if (isnan(x)) {
        return x;
    }
    if (x >= 1024.0) {
        errno = ERANGE;
        return HUGE_VAL;
    }
    if (x <= -1075.0) {
        return 0.0;
    }
    if (x == rint(x) && fabs(x) < 1024.0) {
        return ldexp(1.0, (int)x);
    }
    return exp(x * LN2_HI + x * LN2_LO);
}

double expm1(double x) {
    double x2, p;

    if (isnan(x)) {
        return x;
    }
    if (x > 709.782712893384) {
        errno = ERANGE;
        return HUGE_VAL;
    }

    if (fabs(x) <= 0.5) {
        x2 = x * x;
        p = x2 * (1.0 / 2.0 + x2 * (1.0 / 6.0 + x2 * (1.0 / 24.0 +
            x2 * (1.0 / 120.0 + x2 * (1.0 / 720.0 + x2 * (1.0 / 5040.0 +
            x2 * (1.0 / 40320.0 + x2 * (1.0 / 362880.0 + x2 *
            (1.0 / 3628800.0 + x2 * (1.0 / 39916800.0 + x2 *
            (1.0 / 479001600.0 + x2 * (1.0 / 6227020800.0 + x2 *
            (1.0 / 87178291200.0 + x2 / 1307674368000.0)))))))))))));
        return x + p;
    }
    return exp(x) - 1.0;
}

double log(double x) {
    double m, s, z, r, w;
    int e;
    static const double c[11] = {
        1.0 / 21.0, 1.0 / 19.0, 1.0 / 17.0, 1.0 / 15.0, 1.0 / 13.0,
        1.0 / 11.0, 1.0 / 9.0, 1.0 / 7.0, 1.0 / 5.0, 1.0 / 3.0, 1.0
    };
    int i;

    if (isnan(x)) {
        return x;
    }
    if (x < 0.0) {
        errno = EDOM;
        return DNAN;
    }
    if (x == 0.0) {
        errno = ERANGE;
        return -HUGE_VAL;
    }
    if (isinf(x)) {
        return x;
    }

    m = frexp(x, &e);
    if (m < 0.70710678118654752440) {
        m *= 2.0;
        e--;
    }
    s = (m - 1.0) / (m + 1.0);
    z = s * s;
    r = c[0];
    for (i = 1; i < 11; i++) {
        r = r * z + c[i];
    }
    w = 2.0 * s * r;
    return w + (double)e * LN2_HI + (double)e * LN2_LO;
}

double log2(double x) {
    double m, s, z, r, w;
    int e;
    static const double c[11] = {
        1.0 / 21.0, 1.0 / 19.0, 1.0 / 17.0, 1.0 / 15.0, 1.0 / 13.0,
        1.0 / 11.0, 1.0 / 9.0, 1.0 / 7.0, 1.0 / 5.0, 1.0 / 3.0, 1.0
    };
    int i;

    if (isnan(x)) {
        return x;
    }
    if (x < 0.0) {
        errno = EDOM;
        return DNAN;
    }
    if (x == 0.0) {
        errno = ERANGE;
        return -HUGE_VAL;
    }
    if (isinf(x)) {
        return x;
    }

    m = frexp(x, &e);
    if (m < 0.70710678118654752440) {
        m *= 2.0;
        e--;
    }
    s = (m - 1.0) / (m + 1.0);
    z = s * s;
    r = c[0];
    for (i = 1; i < 11; i++) {
        r = r * z + c[i];
    }
    w = 2.0 * s * r * INV_LN2;
    return (double)e + w;
}

double log10(double x) {
    return log(x) * 0.43429448190325182765;
}

double log1p(double x) {
    double x2, p;

    if (isnan(x)) {
        return x;
    }
    if (fabs(x) > 1e-4) {
        return log(1.0 + x);
    }
    x2 = x * x;
    p = x2 * (-1.0 / 2.0 + x * (1.0 / 3.0 + x * (-1.0 / 4.0 +
        x * (1.0 / 5.0 + x * (-1.0 / 6.0 + x * (1.0 / 7.0 +
        x * (-1.0 / 8.0)))))));
    return x + p;
}

double pow(double x, double y) {
    int neg = 0;
    double r;

    if (y == 0.0) {
        return 1.0;
    }
    if (isnan(x) || isnan(y)) {
        return DNAN;
    }
    if (x == 1.0) {
        return 1.0;
    }
    if (x == 0.0) {
        if (signbit(x) && y == rint(y) && ((long long)y & 1)) {
            neg = 1;
        }
        if (y < 0.0) {
            errno = ERANGE;
            return neg ? -HUGE_VAL : HUGE_VAL;
        }
        return neg ? -0.0 : 0.0;
    }
    if (isinf(y)) {
        double ax = fabs(x);

        if (ax == 1.0) {
            return 1.0;
        }
        if ((ax > 1.0) == (y > 0.0)) {
            return HUGE_VAL;
        }
        return 0.0;
    }
    if (isinf(x)) {
        if (x > 0.0) {
            return (y > 0.0) ? HUGE_VAL : 0.0;
        }
        if (y == rint(y)) {
            int odd = (int)((long long)rint(y) & 1);
            if (y > 0.0) {
                return odd ? -HUGE_VAL : HUGE_VAL;
            }
            return odd ? -0.0 : 0.0;
        }
        errno = EDOM;
        return DNAN;
    }

    if (x < 0.0) {
        if (y != rint(y)) {
            errno = EDOM;
            return DNAN;
        }
        neg = (int)((long long)rint(y) & 1);
        x = -x;
    }

    /* Exponente entero: elevacion por cuadrados (exacta en potencias de dos). */
    if (y == rint(y) && fabs(y) < 2147483648.0) {
        long n = (long)y;
        unsigned long un = (n < 0) ? (unsigned long)(-n) : (unsigned long)n;
        double b = x;

        r = 1.0;
        while (un != 0) {
            if (un & 1u) {
                r *= b;
            }
            b *= b;
            un >>= 1;
        }
        if (n < 0) {
            r = 1.0 / r;
        }
        return neg ? -r : r;
    }

    r = exp2(y * log2(x));
    return neg ? -r : r;
}

float powf(float x, float y) { return (float)pow((double)x, (double)y); }

float expf(float x) { return (float)exp((double)x); }
float exp2f(float x) { return (float)exp2((double)x); }
float expm1f(float x) { return (float)expm1((double)x); }
float logf(float x) { return (float)log((double)x); }
float log2f(float x) { return (float)log2((double)x); }
float log10f(float x) { return (float)log10((double)x); }
float log1pf(float x) { return (float)log1p((double)x); }

long double expl(long double x) { return (long double)exp((double)x); }
long double exp2l(long double x) { return (long double)exp2((double)x); }
long double expm1l(long double x) { return (long double)expm1((double)x); }
long double logl(long double x) { return (long double)log((double)x); }
long double log2l(long double x) { return (long double)log2((double)x); }
long double log10l(long double x) { return (long double)log10((double)x); }
long double log1pl(long double x) { return (long double)log1p((double)x); }
long double powl(long double x, long double y) {
    return (long double)pow((double)x, (double)y);
}

/* ------------------------------------------------------------------ */
/* Trigonometricas (reduccion + polinomios tipo fdlibm)                 */
/* ------------------------------------------------------------------ */

#define PIO2_HI  1.57079632679489655800e+00
#define PIO2_LO  6.12323399573676603587e-17
#define PIO4     7.85398163397448278999e-01
#define INV_PIO2 6.36619772367581382433e-01

static double kernel_sin(double x) {
    static const double S1 = -1.66666666666666324348e-01;
    static const double S2 =  8.33333333332248946124e-03;
    static const double S3 = -1.98412698298579493134e-04;
    static const double S4 =  2.75573137070700676789e-06;
    static const double S5 = -2.50507602534068634195e-08;
    static const double S6 =  1.58969099521155010221e-10;
    double z = x * x;
    double v = z * x;
    double r = S2 + z * (S3 + z * (S4 + z * (S5 + z * S6)));

    return x + v * (S1 + z * r);
}

static double kernel_cos(double x) {
    static const double C1 =  4.16666666666666019037e-02;
    static const double C2 = -1.38888888888741095749e-03;
    static const double C3 =  2.48015872894767294178e-05;
    static const double C4 = -2.75573143513906633035e-07;
    static const double C5 =  2.08757232129817482790e-09;
    static const double C6 = -1.13596475577881948265e-11;
    double z = x * x;
    double r = z * (C1 + z * (C2 + z * (C3 + z * (C4 + z * (C5 + z * C6)))));
    double hz = 0.5 * z;
    double w = 1.0 - hz;

    return w + (((1.0 - w) - hz) + z * r);
}

double sin(double x) {
    double n, r;
    long long q;

    if (isnan(x)) {
        return x;
    }
    if (isinf(x)) {
        errno = EDOM;
        return DNAN;
    }
    if (fabs(x) <= PIO4) {
        return kernel_sin(x);
    }

    n = rint(x * INV_PIO2);
    r = x - n * PIO2_HI - n * PIO2_LO;
    q = (long long)n & 3;
    switch (q) {
    case 0:
        return kernel_sin(r);
    case 1:
        return kernel_cos(r);
    case 2:
        return -kernel_sin(r);
    default:
        return -kernel_cos(r);
    }
}

double cos(double x) {
    double n, r;
    long long q;

    if (isnan(x)) {
        return x;
    }
    if (isinf(x)) {
        errno = EDOM;
        return DNAN;
    }
    if (fabs(x) <= PIO4) {
        return kernel_cos(x);
    }

    n = rint(x * INV_PIO2);
    r = x - n * PIO2_HI - n * PIO2_LO;
    q = (long long)n & 3;
    switch (q) {
    case 0:
        return kernel_cos(r);
    case 1:
        return -kernel_sin(r);
    case 2:
        return -kernel_cos(r);
    default:
        return kernel_sin(r);
    }
}

double tan(double x) {
    /* Cociente seno/coseno tras la misma reduccion.  Pierde precision muy
     * cerca de los polos (donde tan crece sin limite), suficiente para
     * compiladores y programas normales. */
    double c;

    if (isnan(x) || isinf(x)) {
        return DNAN;
    }
    c = cos(x);
    if (c == 0.0) {
        errno = ERANGE;
        return copysign(HUGE_VAL, sin(x));
    }
    return sin(x) / c;
}

double atan(double x) {
    static const double aT[11] = {
        3.33333333333329318027e-01, -1.99999999998764832476e-01,
        1.42857142725034663711e-01, -1.11111104054623557880e-01,
        9.09088713343650656196e-02, -7.69187620504482999495e-02,
        6.66107313738753120669e-02, -5.83357013379057348645e-02,
        4.97687799461593236017e-02, -3.65315727442169155270e-02,
        1.62858201153657823623e-02
    };
    static const double atanhi[4] = {
        4.63647609000806093515e-01, 7.85398163397448278999e-01,
        9.82793723247329054082e-01, 1.57079632679489655800e+00
    };
    static const double atanlo[4] = {
        2.26987774529616870924e-17, 3.06161699786838301793e-17,
        1.39033110312309984516e-17, 6.12323399573676603587e-17
    };
    double ax, z, w, s1, s2;
    int id, neg = 0;

    if (isnan(x)) {
        return x;
    }
    if (x < 0.0) {
        x = -x;
        neg = 1;
    }
    ax = x;

    if (ax >= 1.0e300) {
        id = 3;
        x = 0.0;
    } else if (ax < 0.4375) {
        id = -1;
    } else if (ax < 0.6875) {
        id = 0;
        x = (2.0 * ax - 1.0) / (2.0 + ax);
    } else if (ax < 1.1875) {
        id = 1;
        x = (ax - 1.0) / (ax + 1.0);
    } else if (ax < 2.4375) {
        id = 2;
        x = (ax - 1.5) / (1.0 + 1.5 * ax);
    } else {
        id = 3;
        x = -1.0 / ax;
    }

    if (id == -1) {
        z = x * x;
        w = z * z;
        s1 = z * (aT[0] + w * (aT[2] + w * (aT[4] + w * (aT[6] + w * (aT[8] +
             w * aT[10])))));
        s2 = w * (aT[1] + w * (aT[3] + w * (aT[5] + w * (aT[7] + w * aT[9]))));
        z = x - x * (s1 + s2);
    } else {
        z = x * x;
        w = z * z;
        s1 = z * (aT[0] + w * (aT[2] + w * (aT[4] + w * (aT[6] + w * (aT[8] +
             w * aT[10])))));
        s2 = w * (aT[1] + w * (aT[3] + w * (aT[5] + w * (aT[7] + w * aT[9]))));
        z = atanhi[id] - ((x * (s1 + s2) - atanlo[id]) - x);
    }
    return neg ? -z : z;
}

double asin(double x) {
    static const double pS0 =  1.66666666666666657415e-01;
    static const double pS1 = -3.25565818622400915405e-01;
    static const double pS2 =  2.01212532134862925881e-01;
    static const double pS3 = -4.00555345006794114027e-02;
    static const double pS4 =  7.91534994289814532176e-04;
    static const double pS5 =  3.47933107596021167570e-05;
    static const double qS1 = -2.40339491173441421878e+00;
    static const double qS2 =  2.02094576023350569471e+00;
    static const double qS3 = -6.88283971605453293030e-01;
    static const double qS4 =  7.70381505559019352791e-02;
    double ax, t, w, p, q, r, s;

    if (isnan(x)) {
        return x;
    }
    ax = fabs(x);
    if (ax > 1.0) {
        errno = EDOM;
        return DNAN;
    }
    if (ax == 1.0) {
        return copysign(PIO2_HI + PIO2_LO, x);
    }
    if (ax < 0.5) {
        t = x * x;
        p = t * (pS0 + t * (pS1 + t * (pS2 + t * (pS3 + t * (pS4 + t * pS5)))));
        q = 1.0 + t * (qS1 + t * (qS2 + t * (qS3 + t * qS4)));
        w = p / q;
        return x + x * w;
    }

    /* 0.5 <= |x| < 1: asin(x) = pi/2 - 2*asin(sqrt((1-|x|)/2)) */
    w = 1.0 - ax;
    t = w * 0.5;
    p = t * (pS0 + t * (pS1 + t * (pS2 + t * (pS3 + t * (pS4 + t * pS5)))));
    q = 1.0 + t * (qS1 + t * (qS2 + t * (qS3 + t * qS4)));
    s = sqrt(t);
    r = (p / q) * s;
    r = PIO2_HI - (2.0 * (s + r) - PIO2_LO);
    return copysign(r, x);
}

double acos(double x) {
    double ax = fabs(x);

    if (isnan(x)) {
        return x;
    }
    if (ax > 1.0) {
        errno = EDOM;
        return DNAN;
    }
    if (x == 1.0) {
        return 0.0;
    }
    if (x == -1.0) {
        return M_PI;
    }
    if (ax < 0.5) {
        /* acos(x) = pi/2 - asin(x) */
        return PIO2_HI - asin(x);
    }
    if (x > 0.0) {
        /* acos(x) = 2*asin(sqrt((1-x)/2)) */
        return 2.0 * asin(sqrt((1.0 - x) * 0.5));
    }
    return M_PI - 2.0 * asin(sqrt((1.0 + x) * 0.5));
}

double atan2(double y, double x) {
    double ax, ay, a;

    if (isnan(x) || isnan(y)) {
        return DNAN;
    }
    if (isinf(x) || isinf(y)) {
        if (isinf(x) && isinf(y)) {
            double r = (x > 0.0) ? M_PI_4 : (3.0 * M_PI_4);
            return copysign(r, y);
        }
        if (isinf(y)) {
            return copysign(M_PI_2, y);
        }
        return copysign(0.0, y);
    }

    ax = fabs(x);
    ay = fabs(y);

    if (x > 0.0) {
        if (y == 0.0) {
            return copysign(0.0, y);
        }
        return atan(y / x);
    }
    if (x < 0.0) {
        a = atan(ay / ax);
        return copysign(M_PI - a, y);
    }

    /* x == 0 */
    if (y > 0.0) {
        return M_PI_2;
    }
    if (y < 0.0) {
        return -M_PI_2;
    }
    return copysign(0.0, y);
}

float sinf(float x) { return (float)sin((double)x); }
float cosf(float x) { return (float)cos((double)x); }
float tanf(float x) { return (float)tan((double)x); }
float asinf(float x) { return (float)asin((double)x); }
float acosf(float x) { return (float)acos((double)x); }
float atanf(float x) { return (float)atan((double)x); }
float atan2f(float y, float x) { return (float)atan2((double)y, (double)x); }

long double sinl(long double x) { return (long double)sin((double)x); }
long double cosl(long double x) { return (long double)cos((double)x); }
long double tanl(long double x) { return (long double)tan((double)x); }
long double asinl(long double x) { return (long double)asin((double)x); }
long double acosl(long double x) { return (long double)acos((double)x); }
long double atanl(long double x) { return (long double)atan((double)x); }
long double atan2l(long double y, long double x) {
    return (long double)atan2((double)y, (double)x);
}

/* ------------------------------------------------------------------ */
/* Hiperbolicas                                                         */
/* ------------------------------------------------------------------ */

double sinh(double x) {
    double ax = fabs(x), t, x2;

    if (isnan(x)) {
        return x;
    }
    if (ax < 0.5) {
        x2 = x * x;
        return x * (1.0 + x2 * (1.0 / 6.0 + x2 * (1.0 / 120.0 + x2 *
               (1.0 / 5040.0 + x2 * (1.0 / 362880.0 + x2 / 39916800.0)))));
    }
    if (ax > 710.0) {
        errno = ERANGE;
        return copysign(HUGE_VAL, x);
    }
    t = exp(ax);
    t = (t - 1.0 / t) * 0.5;
    return copysign(t, x);
}

double cosh(double x) {
    double ax = fabs(x), t, x2;

    if (isnan(x)) {
        return x;
    }
    if (ax < 0.5) {
        x2 = x * x;
        return 1.0 + x2 * (0.5 + x2 * (1.0 / 24.0 + x2 * (1.0 / 720.0 +
               x2 / 40320.0)));
    }
    if (ax > 710.0) {
        errno = ERANGE;
        return HUGE_VAL;
    }
    t = exp(ax);
    return (t + 1.0 / t) * 0.5;
}

double tanh(double x) {
    double ax = fabs(x), t, x2;

    if (isnan(x)) {
        return x;
    }
    if (ax < 0.5) {
        x2 = x * x;
        return x * (1.0 - x2 * (1.0 / 3.0 - x2 * (2.0 / 15.0 - x2 *
               (17.0 / 315.0 - x2 * (62.0 / 2835.0 - x2 * (1382.0 / 155925.0))))));
    }
    if (ax > 20.0) {
        return copysign(1.0, x);
    }
    t = exp(2.0 * ax);
    t = (t - 1.0) / (t + 1.0);
    return copysign(t, x);
}

double asinh(double x) {
    double ax = fabs(x), r;

    if (isnan(x)) {
        return x;
    }
    if (ax < 1e-8) {
        return x;
    }
    if (ax < 1e8) {
        r = log(ax + sqrt(ax * ax + 1.0));
    } else {
        r = log(ax) + 0.69314718055994530942;
    }
    return copysign(r, x);
}

double acosh(double x) {
    double t;

    if (isnan(x)) {
        return x;
    }
    if (x < 1.0) {
        errno = EDOM;
        return DNAN;
    }
    if (x == 1.0) {
        return 0.0;
    }
    t = x - 1.0;
    return log1p(t + sqrt(t * (t + 2.0)));
}

double atanh(double x) {
    double ax = fabs(x), x2, r;

    if (isnan(x)) {
        return x;
    }
    if (ax > 1.0) {
        errno = EDOM;
        return DNAN;
    }
    if (ax == 1.0) {
        errno = ERANGE;
        return copysign(HUGE_VAL, x);
    }
    if (ax < 1e-4) {
        x2 = x * x;
        return x * (1.0 + x2 * (1.0 / 3.0 + x2 * (1.0 / 5.0 + x2 * (1.0 / 7.0 +
               x2 * (1.0 / 9.0 + x2 * (1.0 / 11.0 + x2 / 13.0))))));
    }
    r = 0.5 * log((1.0 + x) / (1.0 - x));
    return r;
}

float sinhf(float x) { return (float)sinh((double)x); }
float coshf(float x) { return (float)cosh((double)x); }
float tanhf(float x) { return (float)tanh((double)x); }
float asinhf(float x) { return (float)asinh((double)x); }
float acoshf(float x) { return (float)acosh((double)x); }
float atanhf(float x) { return (float)atanh((double)x); }

long double sinhl(long double x) { return (long double)sinh((double)x); }
long double coshl(long double x) { return (long double)cosh((double)x); }
long double tanhl(long double x) { return (long double)tanh((double)x); }
long double asinhl(long double x) { return (long double)asinh((double)x); }
long double acoshl(long double x) { return (long double)acosh((double)x); }
long double atanhl(long double x) { return (long double)atanh((double)x); }

/* ------------------------------------------------------------------ */
/* Raiz cubica y hipotenusa                                             */
/* ------------------------------------------------------------------ */

double cbrt(double x) {
    double ax, m, r;
    int e, e3, rem, k;

    if (x == 0.0 || isnan(x)) {
        return x;
    }
    if (isinf(x)) {
        return x;
    }
    ax = fabs(x);
    m = frexp(ax, &e);
    e3 = e / 3;
    rem = e - e3 * 3;
    if (rem < 0) {
        rem += 3;
        e3--;
    }
    m = ldexp(m, rem); /* m en [0.5, 4) */

    /* La raiz cubica de m esta en [0.8, 1.6]: Newton converge enseguida. */
    r = 1.0 + (m - 1.0) / 3.0;
    for (k = 0; k < 6; k++) {
        r = r - (r * r * r - m) / (3.0 * r * r);
    }
    return copysign(ldexp(r, e3), x);
}

float cbrtf(float x) { return (float)cbrt((double)x); }
long double cbrtl(long double x) { return (long double)cbrt((double)x); }

double hypot(double x, double y) {
    double a = fabs(x), b = fabs(y), t;

    if (isnan(x) || isnan(y)) {
        return DNAN;
    }
    if (isinf(x) || isinf(y)) {
        return HUGE_VAL;
    }
    if (a < b) {
        t = a;
        a = b;
        b = t;
    }
    if (a == 0.0) {
        return 0.0;
    }
    t = b / a;
    return a * sqrt(1.0 + t * t);
}

float hypotf(float x, float y) { return (float)hypot((double)x, (double)y); }
long double hypotl(long double x, long double y) {
    return (long double)hypot((double)x, (double)y);
}

/* ------------------------------------------------------------------ */
/* Gamma (aproximacion de Lanczos) y error                              */
/* ------------------------------------------------------------------ */

#define LANCZOS_G 7.0

static const double lcz[9] = {
    0.99999999999980993, 676.5203681218851, -1259.1392167224028,
    771.32342877765313, -176.61502916214059, 12.507343278686905,
    -0.13857109526572012, 9.9843695780195716e-6, 1.5056327351493116e-7
};

double lgamma_r(double x, int *signgamp) {
    double a, t, r;
    int i;

    if (signgamp != (int *)0) {
        *signgamp = 1;
    }
    if (isnan(x)) {
        return DNAN;
    }
    if (x <= 0.0 && x == rint(x)) {
        if (signgamp != (int *)0) {
            *signgamp = 1;
        }
        errno = ERANGE;
        return HUGE_VAL;
    }

    if (x < 0.5) {
        /* reflexion: gamma(x)*gamma(1-x) = pi/sin(pi*x) */
        double s = sin(M_PI * x);

        if (signgamp != (int *)0) {
            *signgamp = (s < 0.0) ? -1 : 1;
        }
        return log(M_PI / fabs(s)) - lgamma_r(1.0 - x, (int *)0);
    }

    x -= 1.0;
    a = lcz[0];
    t = x + LANCZOS_G + 0.5;
    for (i = 1; i < 9; i++) {
        a += lcz[i] / (x + (double)i);
    }
    r = 0.5 * log(2.0 * M_PI) + (x + 0.5) * log(t) - t + log(a);
    return r;
}

double lgamma(double x) {
    return lgamma_r(x, (int *)0);
}

double tgamma(double x) {
    int sg = 1;
    double r;

    if (isnan(x)) {
        return x;
    }
    /* Enteros pequenos exactos. */
    if (x > 0.0 && x == rint(x) && x <= 12.0) {
        double f = 1.0;
        int i;

        for (i = 2; i < (int)x; i++) {
            f *= (double)i;
        }
        return (x < 2.0) ? 1.0 : f;
    }
    if (x <= 0.0 && x == rint(x)) {
        errno = EDOM;
        return DNAN;
    }

    r = lgamma_r(x, &sg);
    r = exp(r);
    if (x < 0.0 && sg < 0) {
        r = -r;
    }
    return r;
}

float lgammaf(float x) { return (float)lgamma((double)x); }
float tgammaf(float x) { return (float)tgamma((double)x); }
long double lgammal(long double x) { return (long double)lgamma((double)x); }
long double tgammal(long double x) { return (long double)tgamma((double)x); }

/* erf(x) para |x| < 3 por serie alterna (converge rapido en ese rango). */
static double erf_series(double x) {
    double s = 0.0;
    double t = x;
    int n;

    for (n = 0; n < 120; n++) {
        double term = t / (double)(2 * n + 1);

        s += term;
        t *= -x * x / (double)(n + 1);
        if (fabs(term) < 1e-20) {
            break;
        }
    }
    return 1.12837916709551257390 * s;
}

/* erfc(x) para x >= 3 por fraccion continua (Lentz), mucho mejor que la serie
 * asintotica en este rango:
 *   erfc(x) = exp(-x^2)/sqrt(pi) / (x + (1/2)/(x + 1/(x + (3/2)/(x + ...))))
 */
static double erfc_cf(double x) {
    const double tiny = 1e-300;
    double f = x;
    double C = x;
    double D = 0.0;
    double a = 0.0; /* a_n = n/2 */
    int n;

    for (n = 1; n <= 400; n++) {
        a += 0.5;
        D = x + a * D;
        if (D == 0.0) {
            D = tiny;
        }
        C = x + a / C;
        if (C == 0.0) {
            C = tiny;
        }
        D = 1.0 / D;
        f *= (C * D);
        if (fabs(C * D - 1.0) < 1e-18) {
            break;
        }
    }
    return exp(-x * x) / (1.77245385090551602730 * f);
}

double erf(double x) {
    double ax = fabs(x);

    if (isnan(x)) {
        return x;
    }
    if (isinf(x)) {
        return copysign(1.0, x);
    }
    if (ax < 3.0) {
        return copysign(erf_series(ax), x);
    }
    return copysign(1.0 - erfc_cf(ax), x);
}

double erfc(double x) {
    double ax;

    if (isnan(x)) {
        return x;
    }
    if (x < 0.0) {
        return 2.0 - erfc(-x);
    }
    ax = x;
    if (ax < 3.0) {
        return 1.0 - erf_series(ax);
    }
    return erfc_cf(ax);
}

float erff(float x) { return (float)erf((double)x); }
float erfcf(float x) { return (float)erfc((double)x); }
long double erfl(long double x) { return (long double)erf((double)x); }
long double erfcl(long double x) { return (long double)erfc((double)x); }

double nan(const char *tag) {
    (void)tag;
    return DNAN;
}

float nanf(const char *tag) {
    (void)tag;
    return __builtin_nanf("");
}

long double nanl(const char *tag) {
    (void)tag;
    return __builtin_nanl("");
}
