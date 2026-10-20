#include <stdio.h>

/*
 * fmt.c - prueba del motor de formato de MiOS (cons_printf / printf).
 *
 * Recoge todos los formatos que usa el shell y los comandos, para detectar
 * regresiones sin necesidad de arrancar MiOS.
 *
 *   ./mios-gcc examples/fmt.c -o fmt && sudo chroot sysroot /fmt
 */

int main(void) {
    printf("=== prueba del motor de formato ===\n");

    printf("cadenas      : [%s] [%10s] [%-10s]\n", "abc", "abc", "abc");
    printf("caracter     : [%c] [%c]\n", 'X', 66);
    printf("enteros      : %d %d %d\n", 0, -7, 2147483647);
    printf("sin signo    : %u %lu\n", 4294967295u, 1234567890123ul);
    printf("hexadecimal  : %x %x\n", 48879, 255);
    printf("ancho y cero : [%02d] [%5d] [%-5d] [%08d]\n", 6, 42, 42, 42);
    printf("largos       : %ld %lu\n", -1234567890123456789L, 1234567890123456789ul);
    printf("porcentaje   : %%\n");
    printf("mezcla       : %-8s %s %d %u\n", "modulo", "OK", 34, 34u);
    printf("fin\n");

    return 0;
}
