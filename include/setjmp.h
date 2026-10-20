#ifndef MIOS_SETJMP_H
#define MIOS_SETJMP_H

/*
 * MiOS - setjmp.h (cabecera publica).
 *
 * jmp_buf es una estructura propia de MiOS (no la de glibc): setjmp() y
 * longjmp() las implementa src/lib/setjmp.c y solo tienen que ser coherentes
 * entre si.  Guardan los registros que la ABI x86_64 conserva entre llamadas
 * (rbx, rbp, r12-r15), el puntero de pila y la direccion de retorno.
 */

#define MIOS_JB_REGS 8

typedef struct mios_jmp_buf_tag {
    unsigned long __regs[MIOS_JB_REGS]; /* rbx rbp r12 r13 r14 r15 rsp rip */
} jmp_buf[1];

int  setjmp(jmp_buf env) __attribute__((returns_twice));
void longjmp(jmp_buf env, int val) __attribute__((noreturn));

#endif /* MIOS_SETJMP_H */
