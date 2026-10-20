#include "lib/lib.h"
#include "kernel/syscall.h"
#include "../../include/setjmp.h"  /* cabecera publica: comprobacion de firmas */

/*
 * MiOS - setjmp/longjmp.
 *
 * Salto no local: guardar (setjmp) y restaurar (longjmp) el estado de una
 * funcion.  Se guardan solo los registros que la ABI x86_64 conserva entre
 * llamadas (rbx, rbp, r12-r15) mas el puntero de pila y la direccion de
 * retorno; los registros de argumentos y los volatiles no sobreviven a una
 * llamada, asi que no hay que salvarlos.
 *
 * No se usa el formato de glibc: jmp_buf es propio de MiOS y solo tiene que
 * ser coherente entre setjmp() y longjmp().
 */

int setjmp(jmp_buf env) {
    unsigned long *e = env[0].__regs;

    __asm__ __volatile__(
        "movq %%rbx,  0(%0)\n\t"
        "movq %%rbp,  8(%0)\n\t"
        "movq %%r12, 16(%0)\n\t"
        "movq %%r13, 24(%0)\n\t"
        "movq %%r14, 32(%0)\n\t"
        "movq %%r15, 40(%0)\n\t"
        "leaq 8(%%rsp), %%rax\n\t"  /* pila tal como queda al volver aqui */
        "movq %%rax, 48(%0)\n\t"
        "movq (%%rsp), %%rax\n\t"   /* direccion de retorno */
        "movq %%rax, 56(%0)\n\t"
        :
        : "r"(e)
        : "rax", "memory");

    return 0;
}

void longjmp(jmp_buf env, int val) {
    /* Los operandos van en registros concretos que NO estan en la lista de
     * destrozo, para que el compilador no los ponga en rbx/rbp/r12-r15. */
    register unsigned long base __asm__("rsi") = (unsigned long)env[0].__regs;
    register unsigned long sp   __asm__("rdi") = env[0].__regs[6];
    register unsigned long ret  __asm__("rcx") = (unsigned long)val;
    register unsigned long pc   __asm__("rdx") = env[0].__regs[7];

    __asm__ __volatile__(
        "movq %2, %%rax\n\t"        /* rax = valor que devolvera setjmp */
        "movq  0(%0), %%rbx\n\t"
        "movq  8(%0), %%rbp\n\t"
        "movq 16(%0), %%r12\n\t"
        "movq 24(%0), %%r13\n\t"
        "movq 32(%0), %%r14\n\t"
        "movq 40(%0), %%r15\n\t"
        "movq %1, %%rsp\n\t"
        "jmp *%3\n\t"
        :
        : "r"(base), "r"(sp), "r"(ret), "r"(pc)
        : "rax", "rbx", "rbp", "r12", "r13", "r14", "r15", "memory");

    __builtin_unreachable();
}
