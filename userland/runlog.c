#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <mios.h>

/*
 * runlog - ejecuta un programa y copia su salida al puerto serie.
 *
 * En MiOS, el stdout/stderr de un programa es el de la consola del sistema
 * (fd 1 y fd 2 = /dev/console = pantalla VGA); solo los mensajes que MiOS
 * escribe con cons_* se copian ademas al puerto serie (src/lib/console.c).
 *
 * Este programa resuelve el caso de querer ver (o dejar registrada) la salida
 * de un programa cualquiera: le pone una tuberia como stdout/stderr, la lee y
 * la escribe en los dos sitios, la pantalla y /dev/ttyS0:
 *
 *     MiOS> runlog tcc -o /tmp/hola /usr/src/hola.c
 *     MiOS> runlog /tmp/hola
 *
 * Termina con el mismo codigo que el programa, asi que el shell lo reporta
 * igual que cualquier otra orden.
 */

int main(int argc, char **argv) {
    int fds[2];
    int serial;
    int st = 0;
    pid_t pid;
    char buf[512];

    if (argc < 2) {
        cons_puts("uso: runlog <programa> [argumentos...]\n");
        return 2;
    }

    if (pipe(fds) != 0) {
        cons_puts("runlog: no se pudo crear la tuberia\n");
        return 2;
    }

    /* El puerto serie puede no existir (o no ser escribible): se sigue sin el. */
    serial = open("/dev/ttyS0", O_WRONLY);

    pid = fork();
    if (pid == 0) {
        /* Hijo: su stdout y su stderr van a la tuberia. */
        close(fds[0]);
        dup2(fds[1], 1);
        dup2(fds[1], 2);
        if (fds[1] > 2) {
            close(fds[1]);
        }
        execvp(argv[1], &argv[1]);
        _exit(127);
    }

    close(fds[1]);

    /* Padre: copia lo que salga de la tuberia a la pantalla y al puerto serie. */
    for (;;) {
        long n = read(fds[0], buf, sizeof(buf));
        if (n <= 0) {
            break;
        }
        write(1, buf, (size_t)n);
        if (serial >= 0) {
            write(serial, buf, (size_t)n);
        }
    }
    close(fds[0]);

    waitpid(pid, &st, 0);
    if (serial >= 0) {
        close(serial);
    }

    if (WIFEXITED(st)) {
        st = WEXITSTATUS(st);
    } else {
        st = 128;
    }

    cons_printf("runlog: '%s' termino con codigo %d\n", argv[1], st);
    cons_flush();

    return st;
}
