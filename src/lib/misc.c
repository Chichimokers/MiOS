#include "lib/lib.h"
#include "lib/ret.h"
#include "kernel/syscall.h"
#include "../../include/mios.h"
#include "../../include/assert.h"

/*
 * MiOS - piezas sueltas que el codigo compilado espera encontrar.
 */

/* Proteccion de pila (compilamos con -fno-stack-protector, pero el codigo
 * portado puede venir con ella activada). */
unsigned long __stack_chk_guard = 0x00000aff0ddba11UL;

void __stack_chk_fail(void) {
    cons_puts("MiOS: pila corrompida (stack smashing)\n");
    cons_flush();
    mios_exit(134);
}

void __assert_fail(const char *expr, const char *file, unsigned int line,
                   const char *func) {
    fflush((FILE *)0);
    fprintf(stderr, "%s:%u: %s: fallo de assert: %s\n", file, line, func,
            expr);
    fflush((FILE *)0);
    cons_flush();
    mios_exit(134);
}
