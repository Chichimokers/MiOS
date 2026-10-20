#include "mios.h"
#include "../../include/unistd.h"
#include "../../include/sys/wait.h"
#include "../../include/stdio.h"
#include "../../include/errno.h"
#include "../../include/stdlib.h"

/*
 * MiOS - ejecucion de programas externos (Fase 4).
 *
 * El shell de MiOS es el PID 1; para lanzar un programa hace fork() y en el
 * hijo execvp().  El padre espera con waitpid() y devuelve el codigo de salida.
 * Los descriptores 0/1/2 que el kernel le dio al PID 1 los hereda el hijo, asi
 * que su salida aparece en la misma consola.
 *
 * Antes de bifurcar se vacian los buffers (consola y stdio): si no, el hijo
 * heredaria una copia del buffer y lo escribiria por duplicado.
 */

#define PROG_MAX_ARGS 32

/* Trocea `args` en palabras (sin comillas: como el shell de MiOS). */
static int prog_split(char *args, char *argv[], int max) {
    int argc = 0;
    char *p = args;

    while (*p != '\0' && argc < max) {
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        if (*p == '\0') {
            break;
        }
        argv[argc++] = p;
        while (*p != '\0' && *p != ' ' && *p != '\t') {
            p++;
        }
        if (*p != '\0') {
            *p = '\0';
            p++;
        }
    }
    argv[argc] = MIOS_NULL;
    return argc;
}

int prog_run(const char *cmd, char *args) {
    char *argv[PROG_MAX_ARGS + 2];
    pid_t pid;
    int status = 0;

    argv[0] = (char *)cmd;
    prog_split(args, argv + 1, PROG_MAX_ARGS);

    cons_flush();
    fflush(MIOS_NULL);

    pid = fork();
    if (pid < 0) {
        cons_printf("MiOS: no se pudo crear el proceso: %s\n", strerror(errno));
        return -1;
    }

    if (pid == 0) {
        /* Hijo: si execvp vuelve, es que fallo. */
        execvp(cmd, argv);
        cons_printf("MiOS: no se pudo ejecutar '%s': %s\n", cmd, strerror(errno));
        cons_flush();
        _exit(127);
    }

    if (waitpid(pid, &status, 0) < 0) {
        cons_printf("MiOS: waitpid: %s\n", strerror(errno));
        return -1;
    }

    if (WIFEXITED(status)) {
        int rc = WEXITSTATUS(status);

        /* Como un shell: si el programa termina bien, silencio; si no, se dice
         * el codigo (util para scripts y para /etc/autorun). */
        if (rc != 0) {
            cons_printf("MiOS: '%s' termino con codigo %d\n", cmd, rc);
        }
        return rc;
    }
    if (WIFSIGNALED(status)) {
        cons_printf("MiOS: '%s' terminado por la senal %d\n", cmd, WTERMSIG(status));
        return 128 + WTERMSIG(status);
    }
    return -1;
}
