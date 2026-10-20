/*
 * args.c - imprime sus argumentos y el entorno, y termina con codigo = argc.
 *
 * Sirve para comprobar desde la shell de MiOS que los argumentos y el codigo
 * de salida llegan al padre.
 */

#include <mios.h>

int main(int argc, char **argv) {
    const char *path = getenv("PATH");
    int i;

    printf("args: %d argumento(s)\n", argc);
    for (i = 0; i < argc; i++) {
        printf("  argv[%d] = \"%s\"\n", i, argv[i]);
    }
    printf("  PATH = %s\n", (path != NULL) ? path : "(sin PATH)");
    printf("saliendo con codigo %d\n", argc);

    return argc;
}
