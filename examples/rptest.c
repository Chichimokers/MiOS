/*
 * rptest.c - prueba de realpath()/canonicalize_file_name() de MiOS.
 *
 * Es la funcion que necesita binutils (libiberty la usa para comprobar que la
 * entrada y la salida de 'ld' no son el mismo fichero).  Se prueban rutas
 * absolutas, con "." y "..", con barras repetidas, enlaces simbolicos y rutas
 * que no existen.  Se ejecuta dentro del sysroot:
 *
 *   bash mios-gcc examples/rptest.c -o rptest
 *   cp rptest sysroot/ && sudo chroot sysroot /rptest
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int fallos = 0;

static void espera(const char *path, const char *quiero) {
    char buf[4096];
    char *r = realpath(path, buf);
    int ok = (r != NULL) && (strcmp(r, quiero) == 0);

    if (!ok) {
        fallos++;
    }
    printf("realpath(\"%s\") = \"%s\" %s (esperaba \"%s\")\n",
           path, r ? r : "(NULL)", ok ? "OK" : "MAL", quiero);
}

static void reservado(const char *path) {
    char *r = canonicalize_file_name(path);

    printf("canonicalize_file_name(\"%s\") = \"%s\" %s\n",
           path, r ? r : "(NULL)", (r != NULL) ? "OK" : "MAL");
    if (r == NULL) {
        fallos++;
    }
    free(r);
}

static void debe_fallar(const char *path) {
    char buf[4096];
    char *r = realpath(path, buf);

    printf("realpath(\"%s\") = %s %s (debia fallar)\n",
           path, r ? r : "(NULL)", (r == NULL) ? "OK" : "MAL");
    if (r != NULL) {
        fallos++;
    }
}

int main(void) {
    char cwd[4096];

    printf("rptest: realpath de MiOS\n");

    espera("/", "/");
    espera("/lib", "/lib");
    espera("/lib/libmios.so.6", "/lib/libmios.so.6");
    espera("/lib/./libmios.so.6", "/lib/libmios.so.6");
    espera("/lib/../lib//libmios.so.6", "/lib/libmios.so.6");
    espera("/lib/../..", "/");
    espera("/lib/../lib/../lib", "/lib");

    /* /lib/libmios.so es un enlace relativo -> libmios.so.6 */
    espera("/lib/libmios.so", "/lib/libmios.so.6");

    debe_fallar("/no/existe/ni/de/aqui");
    debe_fallar("");

    reservado("/lib/libmios.so");

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        strcpy(cwd, "?");
    }
    printf("cwd = \"%s\"\n", cwd);
    chdir("/lib");
    espera("./libmios.so.6", "/lib/libmios.so.6");
    espera("../lib/./libmios.so.6", "/lib/libmios.so.6");
    espera("..", "/");
    chdir(cwd);

    printf("rptest: %d fallo(s)\n", fallos);
    return fallos == 0 ? 0 : 1;
}
