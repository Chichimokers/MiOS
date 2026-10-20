/*
 * fase3.c - prueba de la biblioteca matematica de MiOS (Fase 3).
 *
 * Ejercita math.h (double, float y long double) y fenv.h.  Los resultados se
 * comparan con valores de referencia con tolerancias de doble precision.
 *
 * Termina con un resumen; el codigo de salida es el numero de fallos.
 */

#include <mios.h>
#include <math.h>
#include <fenv.h>

static int fallos = 0;

static double absd(double x) {
    return (x < 0.0) ? -x : x;
}

static void check(int cond, const char *que) {
    if (cond) {
        printf("  ok    %s\n", que);
    } else {
        printf("  FALLO %s\n", que);
        fallos++;
    }
}

static void cerca(double got, double esp, double tol, const char *que) {
    double d = absd(got - esp);

    if (d <= tol) {
        printf("  ok    %s (%.10f)\n", que, got);
    } else {
        printf("  FALLO %s: %.10f, esperado %.10f (tolerancia %.1e)\n",
               que, got, esp, tol);
        fallos++;
    }
}

static void cerca_rel(double got, double esp, double rel, const char *que) {
    double d = absd(got - esp);
    double m = absd(esp);

    if (m < 1e-300) {
        m = 1e-300;
    }
    if (d <= rel * m) {
        printf("  ok    %s (%.10e)\n", que, got);
    } else {
        printf("  FALLO %s: %.10e, esperado %.10e (rel %.1e)\n",
               que, got, esp, rel);
        fallos++;
    }
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    printf("== MiOS Fase 3: libm propia + fenv ==\n");

    /* --- raices y valor absoluto --- */
    printf("[raices]\n");
    cerca(sqrt(4.0), 2.0, 0.0, "sqrt(4)");
    cerca(sqrt(2.0), 1.4142135623730951, 1e-16, "sqrt(2)");
    cerca(sqrtf(9.0f), 3.0, 1e-6, "sqrtf(9)");
    cerca((double)sqrtl(16.0L), 4.0, 1e-15, "sqrtl(16)");
    cerca(cbrt(27.0), 3.0, 1e-15, "cbrt(27)");
    cerca(cbrt(-8.0), -2.0, 1e-15, "cbrt(-8)");
    cerca(hypot(3.0, 4.0), 5.0, 1e-15, "hypot(3,4)");
    cerca(fabs(-3.5), 3.5, 0.0, "fabs");
    check(copysign(2.0, -1.0) == -2.0, "copysign");
    check(signbit(-0.0) && !signbit(1.0), "signbit");

    /* --- redondeo --- */
    printf("[redondeo]\n");
    cerca(floor(2.7), 2.0, 0.0, "floor(2.7)");
    cerca(floor(-2.1), -3.0, 0.0, "floor(-2.1)");
    cerca(ceil(2.1), 3.0, 0.0, "ceil(2.1)");
    cerca(trunc(-2.9), -2.0, 0.0, "trunc(-2.9)");
    cerca(round(2.5), 3.0, 0.0, "round(2.5) ties away");
    cerca(round(-2.5), -3.0, 0.0, "round(-2.5) ties away");
    cerca(rint(2.5), 2.0, 0.0, "rint(2.5) ties to even");
    cerca(rint(3.5), 4.0, 0.0, "rint(3.5) ties to even");
    cerca(nearbyint(-2.5), -2.0, 0.0, "nearbyint(-2.5)");
    check(lround(2.5) == 3 && llround(-2.5) == -3, "lround/llround");

    /* --- resto --- */
    printf("[resto]\n");
    cerca(fmod(10.0, 3.0), 1.0, 0.0, "fmod(10,3)");
    cerca(fmod(-10.0, 3.0), -1.0, 0.0, "fmod(-10,3)");
    cerca((double)fmodl(10.0L, 3.0L), 1.0, 1e-18, "fmodl(10,3)");
    cerca(remainder(10.0, 3.0), 1.0, 0.0, "remainder(10,3)");
    cerca(remainder(5.0, 3.0), -1.0, 0.0, "remainder(5,3)");

    /* --- descomposicion --- */
    printf("[descomposicion]\n");
    {
        int e = 0;
        double m = frexp(0.75, &e);
        double ip = 0.0;
        double fr = modf(3.75, &ip);

        check(m == 0.75 && e == 0, "frexp(0.75) -> 0.75, exp 0");
        m = frexp(8.0, &e);
        check(m == 0.5 && e == 4, "frexp(8) -> 0.5, exp 4");
        cerca(ldexp(1.0, 10), 1024.0, 0.0, "ldexp(1,10)");
        cerca(scalbn(3.0, 4), 48.0, 0.0, "scalbn(3,4)");
        check(ip == 3.0 && absd(fr - 0.75) < 1e-16, "modf(3.75)");
        check(ilogb(1024.0) == 10, "ilogb(1024)");
    }

    /* --- exponencial y logaritmos --- */
    printf("[exp/log]\n");
    cerca(exp(0.0), 1.0, 0.0, "exp(0)");
    cerca(exp(1.0), 2.7182818284590452354, 1e-15, "exp(1)");
    cerca(exp2(10.0), 1024.0, 0.0, "exp2(10)");
    cerca(log(2.7182818284590452354), 1.0, 1e-15, "log(e)");
    cerca(log2(1024.0), 10.0, 1e-15, "log2(1024)");
    cerca(log10(1000.0), 3.0, 1e-15, "log10(1000)");
    cerca(log1p(1e-10), 1e-10, 1e-20, "log1p(1e-10)");
    cerca(expm1(1e-10), 1e-10, 1e-20, "expm1(1e-10)");
    check(logb(1024.0) == 10.0, "logb(1024)");
    check(isinf(log(0.0)) && log(0.0) < 0.0, "log(0) = -inf");

    /* --- potencia --- */
    printf("[pow]\n");
    cerca(pow(2.0, 10.0), 1024.0, 0.0, "pow(2,10)");
    cerca(pow(2.0, 0.5), 1.4142135623730951, 1e-15, "pow(2,0.5)");
    cerca(pow(-2.0, 3.0), -8.0, 0.0, "pow(-2,3)");
    cerca(pow(3.0, -1.0), 1.0 / 3.0, 1e-16, "pow(3,-1)");
    cerca(pow(10.0, 3.0), 1000.0, 0.0, "pow(10,3)");
    cerca(powf(3.0f, 3.0f), 27.0, 1e-5, "powf(3,3)");

    /* --- trigonometricas --- */
    printf("[trig]\n");
    cerca(sin(0.0), 0.0, 0.0, "sin(0)");
    cerca(cos(0.0), 1.0, 0.0, "cos(0)");
    cerca(sin(M_PI / 2.0), 1.0, 1e-15, "sin(pi/2)");
    cerca(cos(M_PI), -1.0, 1e-15, "cos(pi)");
    cerca(sin(M_PI / 6.0), 0.5, 1e-16, "sin(pi/6)");
    cerca(tan(M_PI / 4.0), 1.0, 1e-15, "tan(pi/4)");
    cerca(atan(1.0), M_PI / 4.0, 1e-16, "atan(1)");
    cerca(atan2(1.0, 1.0), M_PI / 4.0, 1e-16, "atan2(1,1)");
    cerca(asin(0.5), M_PI / 6.0, 1e-15, "asin(0.5)");
    cerca(acos(0.5), M_PI / 3.0, 1e-15, "acos(0.5)");
    cerca(atan2(1.0, 0.0), M_PI / 2.0, 1e-16, "atan2(1,0)");
    cerca(sinf(1.0f), 0.8414709848078965, 1e-6, "sinf(1)");
    cerca((double)sinl(1.0L), 0.8414709848078965, 1e-15, "sinl(1)");

    /* --- hiperbolicas --- */
    printf("[hiperbolicas]\n");
    cerca(sinh(1.0), 1.1752011936438014, 1e-15, "sinh(1)");
    cerca(cosh(1.0), 1.5430806348152437, 1e-15, "cosh(1)");
    cerca(tanh(1.0), 0.7615941559557649, 1e-15, "tanh(1)");
    cerca(asinh(1.0), 0.8813735870195430, 1e-15, "asinh(1)");
    cerca(acosh(2.0), 1.3169578969248166, 1e-15, "acosh(2)");
    cerca(atanh(0.5), 0.5493061443340549, 1e-15, "atanh(0.5)");

    /* --- gamma y error --- */
    printf("[gamma/error]\n");
    cerca(tgamma(5.0), 24.0, 1e-12, "tgamma(5) = 24");
    cerca(tgamma(0.5), 1.7724538509055160, 1e-14, "tgamma(0.5) = sqrt(pi)");
    cerca(lgamma(5.0), 3.1780538303479458, 1e-14, "lgamma(5) = log(24)");
    cerca(erf(0.0), 0.0, 0.0, "erf(0)");
    cerca(erf(1.0), 0.8427007929497149, 1e-14, "erf(1)");
    cerca(erfc(1.0), 0.15729920705028513, 1e-14, "erfc(1)");

    /* --- clasificacion y varios --- */
    printf("[clasificacion]\n");
    check(isnan(NAN) && isnan(sqrt(-1.0)), "isnan");
    check(isinf(INFINITY) && isinf(log(0.0)), "isinf");
    check(isfinite(1.0) && !isfinite(INFINITY), "isfinite");
    check(fpclassify(0.0) == FP_ZERO && fpclassify(1.0) == FP_NORMAL, "fpclassify");
    check(fpclassify(NAN) == FP_NAN && fpclassify(INFINITY) == FP_INFINITE,
          "fpclassify nan/inf");
    cerca(fmax(3.0, 7.0), 7.0, 0.0, "fmax");
    cerca(fmin(3.0, 7.0), 3.0, 0.0, "fmin");
    cerca(fdim(7.0, 3.0), 4.0, 0.0, "fdim");
    cerca(fma(2.0, 3.0, 4.0), 10.0, 0.0, "fma");
    check(nextafter(1.0, 2.0) > 1.0, "nextafter hacia arriba");
    check(nextafter(1.0, 0.0) < 1.0, "nextafter hacia abajo");
    check(isnan(nan("")) , "nan()");

    /* --- fenv --- */
    printf("[fenv]\n");
    {
        int modo = fegetround();

        check(modo == FE_TONEAREST, "modo de redondeo por defecto: al mas cercano");
        fesetround(FE_DOWNWARD);
        check(fegetround() == FE_DOWNWARD, "fesetround(FE_DOWNWARD)");
        cerca(rint(2.5), 2.0, 0.0, "rint(2.5) con redondeo hacia abajo");
        fesetround(FE_UPWARD);
        cerca(rint(2.5), 3.0, 0.0, "rint(2.5) con redondeo hacia arriba");
        fesetround(FE_TOWARDZERO);
        cerca(rint(-2.5), -2.0, 0.0, "rint(-2.5) hacia cero");
        fesetround(modo);
        check(fegetround() == FE_TONEAREST, "modo restaurado");

        feclearexcept(FE_ALL_EXCEPT);
        check(fetestexcept(FE_ALL_EXCEPT) == 0, "feclearexcept / fetestexcept");

        {
            fenv_t env;

            fegetenv(&env);
            feholdexcept(&env);
            feupdateenv(&env);
            check(1, "fegetenv/feholdexcept/feupdateenv no revientan");
        }
    }

    /* --- valores dificiles (rango amplio) --- */
    printf("[valores dificiles]\n");
    cerca_rel(exp(-700.0), 9.859676543759770e-305, 1e-14, "exp(-700)");
    cerca_rel(exp(700.0), 1.0142320547350045e304, 1e-14, "exp(700)");
    cerca_rel(exp2(-100.0), 7.888609052210118e-31, 1e-15, "exp2(-100)");
    cerca_rel(log(1e300), 690.77552789821368, 1e-15, "log(1e300)");
    cerca_rel(log2(0.001), -9.9657842846620870, 1e-15, "log2(0.001)");
    cerca_rel(pow(1e10, 3.0), 1e30, 1e-14, "pow(1e10,3)");
    cerca_rel(pow(2.0, -1074.0), 4.9406564584124654e-324, 1e-14, "pow(2,-1074)");
    cerca_rel(sin(100.0), -0.50636564110975879, 1e-14, "sin(100)");
    cerca_rel(cos(100.0), 0.86231887228768393, 1e-14, "cos(100)");
    cerca_rel(tan(100.0), -0.58721391515692907, 1e-14, "tan(100)");
    cerca_rel(hypot(1e200, 1e200), 1.4142135623730951e200, 1e-15, "hypot(1e200,1e200)");
    cerca_rel(hypot(1e-300, 1e-300), 1.4142135623730951e-300, 1e-14, "hypot(1e-300,1e-300)");
    cerca_rel(cbrt(1e-30), 1e-10, 1e-14, "cbrt(1e-30)");
    cerca_rel(asinh(1e10), 23.718998110500400, 1e-15, "asinh(1e10)");
    cerca_rel(tgamma(10.0), 362880.0, 1e-13, "tgamma(10)");
    cerca_rel(tgamma(-0.5), -3.5449077018110318, 1e-13, "tgamma(-0.5)");
    cerca_rel(lgamma(100.0), 359.13420536957540, 1e-14, "lgamma(100)");
    cerca_rel(lgamma(-0.5), 1.2655121234846454, 1e-13, "lgamma(-0.5)");
    cerca_rel(erf(0.5), 0.52049987781304654, 1e-14, "erf(0.5)");
    cerca_rel(erf(3.0), 0.99997790950300141, 1e-14, "erf(3)");
    cerca_rel(erfc(3.0), 2.2090496998585441e-5, 1e-12, "erfc(3)");
    cerca_rel(erf(-1.0), -0.8427007929497149, 1e-14, "erf(-1)");
    cerca_rel(erfc(-1.0), 1.8427007929497148, 1e-14, "erfc(-1)");
    cerca_rel(erfc(5.0), 1.5374597944280351e-12, 1e-12, "erfc(5)");
    cerca_rel(erfc(6.0), 2.1519736712498913e-17, 1e-11, "erfc(6)");

    printf("\n== resultado: %d fallo(s) ==\n", fallos);
    return fallos;
}
