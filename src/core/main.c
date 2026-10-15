#include "mios.h"

/*
 * MiOS - punto de entrada.
 *
 * El kernel ejecuta /init como PID 1. _start prepara la pila y llama a
 * mios_entry(); el proceso nunca debe terminar.
 */

__attribute__((naked, noreturn)) void _start(void) {
    __asm__ volatile(
        "xor %rbp, %rbp\n"
        "and $-16, %rsp\n"
        "call mios_entry\n"
        "ud2\n");
}

void mios_entry(void) {
    boot_setup_environment();
    boot_banner();
    boot_load_modules();
    shell_init();
    shell_run();
    mios_exit(0); /* Inalcanzable: el PID 1 nunca termina */
}
