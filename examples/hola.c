#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <mios.h>

/*
 * hola.c - primer programa de MiOS compilado con mios-gcc.
 *
 * No usa la libc del VPS: todo (printf, strlen, uname...) lo resuelve
 * libmios.so.6 a traves de ld-mios.so.1.
 *
 *   ./mios-gcc examples/hola.c -o hola
 */

static void saludar(const char *quien) {
    printf("Hola, %s!\n", quien);
}

int main(int argc, char **argv) {
    struct utsname u;
    int i;

    printf("=== hola desde MiOS ===\n");
    printf("argc = %d\n", argc);
    for (i = 0; i < argc; i++) {
        printf("  argv[%d] = \"%s\"\n", i, argv[i]);
    }

    saludar("libmios");

    if (uname(&u) == 0) {
        printf("uname: %s %s %s\n", u.sysname, u.release, u.machine);
    } else {
        printf("uname: error %d\n", errno);
    }

    printf("strlen(\"abc\") = %u\n", (unsigned)strlen("abc"));
    printf("2 + 3 = %d\n", 2 + 3);

    return 0;
}
