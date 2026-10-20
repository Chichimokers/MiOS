#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <mios.h>

/*
 * tcc-hola.c - programa de prueba de tcc DENTRO de MiOS (Fase 5).
 *
 * build-tcc.sh lo copia al initramfs como /usr/src/hola.c, asi que se puede
 * probar el compilador sin teclear nada:
 *
 *     MiOS> tcc -o /tmp/hola /usr/src/hola.c
 *     MiOS> /tmp/hola
 *
 * Informa con cons_* (consola de MiOS: pantalla *y* puerto serie) para que el
 * arranque se pueda verificar sin mirar la pantalla, y termina con codigo
 * distinto de cero si algo falla (el shell lo dice en voz alta).
 */

static int fallos;

static void check(const char *que, int ok) {
    if (!ok) {
        fallos++;
    }
    cons_printf("  %-30s %s\n", que, ok ? "OK" : "FALLO");
}

int main(int argc, char **argv) {
    char buf[64];
    void *p;
    int i;
    int suma = 0;
    double d;

    /* Esta linea sale por el stdout del programa (fd 1: consola del sistema). */
    printf("Hola: programa compilado con tcc dentro de MiOS (argc=%d)\n", argc);

    cons_puts("\n[tcc] pruebas del codigo generado por tcc:\n");

    for (i = 1; i <= 10; i++) {
        suma += i;
    }
    check("bucle y suma (1..10 = 55)", suma == 55);

    strcpy(buf, "MiOS");
    strcat(buf, "+tcc");
    check("cadenas (strcpy/strcat/strcmp)", strcmp(buf, "MiOS+tcc") == 0);

    d = sqrt(2.0) * sqrt(2.0);
    check("matematicas (sqrt)", d > 1.999 && d < 2.001);

    p = malloc(1000);
    check("malloc/free", p != (void *)0);
    free(p);

    check("sizeof(void *) == 8", sizeof(void *) == 8);
    check("argumentos (argv[0])", argv != (char **)0 && argv[0] != (char *)0);

    cons_printf("[tcc] resultado: %d fallo(s)\n\n", fallos);
    cons_flush();

    return fallos != 0;
}
