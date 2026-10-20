#ifndef MIOS_MATH_H
#define MIOS_MATH_H

/*
 * MiOS - math.h (cabecera publica).
 *
 * La biblioteca matematica vive dentro de libmios.so.6 (no hay libm aparte);
 * `-lm` encuentra un libm.so que apunta a la misma biblioteca.
 *
 * Las rutinas son propias (sin libc): precision de doble IEEE-754 con
 * algoritmos tipo fdlibm (minimax / series). Las variantes long double usan
 * aritmetica x87 (64 bits de mantisa) donde es barato y exacto, y delegan en
 * la version double en las trascendentes.
 */

#define M_E        2.7182818284590452354
#define M_LOG2E    1.4426950408889634074
#define M_LOG10E   0.43429448190325182765
#define M_LN2      0.69314718055994530942
#define M_LN10     2.30258509299404568402
#define M_PI       3.14159265358979323846
#define M_PI_2     1.57079632679489661923
#define M_PI_4     0.78539816339744830962
#define M_1_PI     0.31830988618379067154
#define M_2_PI     0.63661977236758134308
#define M_2_SQRTPI 1.12837916709551257390
#define M_SQRT2    1.41421356237309504880
#define M_SQRT1_2  0.70710678118654752440

#define FP_NAN       0
#define FP_INFINITE  1
#define FP_ZERO      2
#define FP_SUBNORMAL 3
#define FP_NORMAL    4

/*
 * gcc (el compilador de MiOS) trae los builtins de coma flotante: son
 * constantes, exactos y rapidos.  Otros compiladores -tcc, el que corre dentro
 * de MiOS- no los tienen, y para ellos se usan funciones de libmios; es la
 * misma idea que sigue glibc con compiladores que no son gcc.
 */
#ifdef __TINYC__

#define HUGE_VALF  (__mios_huge_valf())
#define HUGE_VAL   (__mios_huge_val())
#define HUGE_VALL  (__mios_huge_vall())
#define INFINITY   (__mios_huge_valf())
#define NAN        (__mios_nan())

#define fpclassify(x) __fpclassify((double)(x))
#define isnan(x)      __mios_isnan((double)(x))
#define isinf(x)      __mios_isinf((double)(x))
#define isfinite(x)   __mios_isfinite((double)(x))
#define isnormal(x)   __mios_isnormal((double)(x))
#define signbit(x)    __signbit((double)(x))

/* Estas seis solo necesitan comparaciones: en IEEE, con un NaN dan falso. */
#define isgreater(x, y)      ((x) > (y))
#define isgreaterequal(x, y) ((x) >= (y))
#define isless(x, y)         ((x) < (y))
#define islessequal(x, y)    ((x) <= (y))
#define islessgreater(x, y)  ((x) < (y) || (x) > (y))
#define isunordered(x, y)    (isnan(x) || isnan(y))

double __mios_huge_val(void);
float  __mios_huge_valf(void);
long double __mios_huge_vall(void);
double __mios_nan(void);
int    __mios_isnan(double x);
int    __mios_isinf(double x);
int    __mios_isfinite(double x);
int    __mios_isnormal(double x);

#else /* gcc y compatibles */

#define HUGE_VALF  (__builtin_huge_valf())
#define HUGE_VAL   (__builtin_huge_val())
#define HUGE_VALL  (__builtin_huge_vall())
#define INFINITY   (__builtin_inff())
#define NAN        (__builtin_nanf(""))

#define fpclassify(x) __builtin_fpclassify(FP_NAN, FP_INFINITE, FP_NORMAL, \
                                           FP_SUBNORMAL, FP_ZERO, (x))
#define isnan(x)      __builtin_isnan(x)
#define isinf(x)      __builtin_isinf(x)
#define isfinite(x)   __builtin_isfinite(x)
#define isnormal(x)   __builtin_isnormal(x)
#define signbit(x)    __builtin_signbit(x)

#define isgreater(x, y)      __builtin_isgreater((x), (y))
#define isgreaterequal(x, y) __builtin_isgreaterequal((x), (y))
#define isless(x, y)         __builtin_isless((x), (y))
#define islessequal(x, y)    __builtin_islessequal((x), (y))
#define islessgreater(x, y)  __builtin_islessgreater((x), (y))
#define isunordered(x, y)    __builtin_isunordered((x), (y))

#endif /* __TINYC__ */

/* Clasificacion como funciones (algunas bibliotecas las llaman asi). */
int __fpclassify(double x);
int __fpclassifyf(float x);
int __fpclassifyl(long double x);
int __signbit(double x);
int __signbitf(float x);
int __signbitl(long double x);

/* --- trigonometricas --- */
double sin(double x);        float sinf(float x);        long double sinl(long double x);
double cos(double x);        float cosf(float x);        long double cosl(long double x);
double tan(double x);        float tanf(float x);        long double tanl(long double x);
double asin(double x);       float asinf(float x);       long double asinl(long double x);
double acos(double x);       float acosf(float x);       long double acosl(long double x);
double atan(double x);       float atanf(float x);       long double atanl(long double x);
double atan2(double y, double x);
float atan2f(float y, float x);
long double atan2l(long double y, long double x);

/* --- hiperbolicas --- */
double sinh(double x);       float sinhf(float x);       long double sinhl(long double x);
double cosh(double x);       float coshf(float x);       long double coshl(long double x);
double tanh(double x);       float tanhf(float x);       long double tanhl(long double x);
double asinh(double x);      float asinhf(float x);      long double asinhl(long double x);
double acosh(double x);      float acoshf(float x);      long double acoshl(long double x);
double atanh(double x);      float atanhf(float x);      long double atanhl(long double x);

/* --- exponencial y logaritmos --- */
double exp(double x);        float expf(float x);        long double expl(long double x);
double exp2(double x);       float exp2f(float x);       long double exp2l(long double x);
double expm1(double x);      float expm1f(float x);      long double expm1l(long double x);
double log(double x);        float logf(float x);        long double logl(long double x);
double log2(double x);       float log2f(float x);       long double log2l(long double x);
double log10(double x);      float log10f(float x);      long double log10l(long double x);
double log1p(double x);      float log1pf(float x);      long double log1pl(long double x);
double logb(double x);       float logbf(float x);       long double logbl(long double x);
int    ilogb(double x);      int ilogbf(float x);        int ilogbl(long double x);

/* --- potencia y raiz --- */
double pow(double x, double y);
float powf(float x, float y);
long double powl(long double x, long double y);
double sqrt(double x);       float sqrtf(float x);       long double sqrtl(long double x);
double cbrt(double x);       float cbrtf(float x);       long double cbrtl(long double x);
double hypot(double x, double y);
float hypotf(float x, float y);
long double hypotl(long double x, long double y);

/* --- redondeo --- */
double ceil(double x);       float ceilf(float x);       long double ceill(long double x);
double floor(double x);      float floorf(float x);      long double floorl(long double x);
double trunc(double x);      float truncf(float x);      long double truncl(long double x);
double round(double x);      float roundf(float x);      long double roundl(long double x);
double nearbyint(double x);  float nearbyintf(float x);  long double nearbyintl(long double x);
double rint(double x);       float rintf(float x);       long double rintl(long double x);
long   lrint(double x);      long lrintf(float x);       long lrintl(long double x);
long long llrint(double x);  long long llrintf(float x); long long llrintl(long double x);
long   lround(double x);     long lroundf(float x);      long lroundl(long double x);
long long llround(double x); long long llroundf(float x);
long long llroundl(long double x);

/* --- resto --- */
double fmod(double x, double y);
float fmodf(float x, float y);
long double fmodl(long double x, long double y);
double remainder(double x, double y);
float remainderf(float x, float y);
long double remainderl(long double x, long double y);
double remquo(double x, double y, int *quo);
float remquof(float x, float y, int *quo);
long double remquol(long double x, long double y, int *quo);

/* --- manipulacion --- */
double copysign(double x, double y);
float copysignf(float x, float y);
long double copysignl(long double x, long double y);
double nextafter(double x, double y);
float nextafterf(float x, float y);
long double nextafterl(long double x, long double y);
double nexttoward(double x, long double y);
float nexttowardf(float x, long double y);
long double nexttowardl(long double x, long double y);
double fabs(double x);       float fabsf(float x);       long double fabsl(long double x);
double frexp(double x, int *e);
float frexpf(float x, int *e);
long double frexpl(long double x, int *e);
double ldexp(double x, int e);
float ldexpf(float x, int e);
long double ldexpl(long double x, int e);
double scalbn(double x, int e);
float scalbnf(float x, int e);
long double scalbnl(long double x, int e);
double scalbln(double x, long e);
float scalblnf(float x, long e);
long double scalblnl(long double x, long e);
double modf(double x, double *iptr);
float modff(float x, float *iptr);
long double modfl(long double x, long double *iptr);
double fdim(double x, double y);
float fdimf(float x, float y);
long double fdiml(long double x, long double y);
double fmax(double x, double y);
float fmaxf(float x, float y);
long double fmaxl(long double x, long double y);
double fmin(double x, double y);
float fminf(float x, float y);
long double fminl(long double x, long double y);
double fma(double x, double y, double z);
float fmaf(float x, float y, float z);
long double fmal(long double x, long double y, long double z);

/* --- gamma y error --- */
double lgamma(double x);     float lgammaf(float x);     long double lgammal(long double x);
double tgamma(double x);     float tgammaf(float x);     long double tgammal(long double x);
double lgamma_r(double x, int *signgamp);
double erf(double x);        float erff(float x);        long double erfl(long double x);
double erfc(double x);       float erfcf(float x);       long double erfcl(long double x);

/* --- generacion --- */
double nan(const char *tag);
float nanf(const char *tag);
long double nanl(const char *tag);

#define FP_ILOGB0   (-2147483647 - 1)
#define FP_ILOGBNAN (-2147483647 - 1)

#endif /* MIOS_MATH_H */
