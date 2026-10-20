/*
 * hola.c - programa de ejemplo instalado en /bin de MiOS (Fase 4).
 *
 * Se compila con mios-gcc y se lanza desde el shell de MiOS.
 */

#include <mios.h>

int main(int argc, char **argv) {
    struct utsname u;

    printf("Hola desde un programa de /bin de MiOS!\n");
    printf("  argc = %d, argv[0] = %s\n", argc, argv[0]);

    if (uname(&u) == 0) {
        printf("  kernel: %s %s %s\n", u.sysname, u.release, u.machine);
    }
    printf("  PID = %d, padre = %d\n", getpid(), getppid());

    return 0;
}
