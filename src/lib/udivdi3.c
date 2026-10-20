#include "mios.h"

/*
 * MiOS - division de 64 bits.
 *
 * El compilador genera llamadas a __udivdi3 / __umoddi3 (y las variantes con
 * signo) para la division de enteros de 64 bits. Normalmente las aporta
 * libgcc, pero al enlazar una libreria compartida libgcc puede dar problemas
 * de PIC, asi que las implementamos nosotros.
 *
 * Algoritmo: division larga bit a bit (correcta, no optimizada).
 */

unsigned long __udivmoddi4(unsigned long a, unsigned long b, unsigned long *rem) {
    unsigned long q = 0;
    unsigned long r = 0;
    int i;

    for (i = 63; i >= 0; i--) {
        r = (r << 1) | ((a >> i) & 1UL);
        if (r >= b) {
            r -= b;
            q |= (1UL << i);
        }
    }
    if (rem != (unsigned long *)0) {
        *rem = r;
    }
    return q;
}

unsigned long __udivdi3(unsigned long a, unsigned long b) {
    return __udivmoddi4(a, b, (unsigned long *)0);
}

unsigned long __umoddi3(unsigned long a, unsigned long b) {
    unsigned long r = 0;
    (void)__udivmoddi4(a, b, &r);
    return r;
}

/* Valor absoluto sin desbordar en LONG_MIN. */
static unsigned long abs_u(long v) {
    if (v < 0) {
        return (unsigned long)(-(v + 1)) + 1UL;
    }
    return (unsigned long)v;
}

long __divdi3(long a, long b) {
    unsigned long q = __udivmoddi4(abs_u(a), abs_u(b), (unsigned long *)0);
    return ((a < 0) != (b < 0)) ? -(long)q : (long)q;
}

long __moddi3(long a, long b) {
    unsigned long r = 0;
    (void)__udivmoddi4(abs_u(a), abs_u(b), &r);
    return (a < 0) ? -(long)r : (long)r;
}
