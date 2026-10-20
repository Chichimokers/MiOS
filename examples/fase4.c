/*
 * fase4.c - prueba de procesos y ejecucion de programas de MiOS (Fase 4).
 *
 * Comprueba fork/exec/wait y pipe, que es exactamente lo que hace el shell
 * para lanzar un programa de /bin.  Se ejecuta en el chroot del sysroot: los
 * programas /hola y /args se copian ahi desde userland/.
 *
 * Termina con un resumen; el codigo de salida es el numero de fallos.
 */

#include <mios.h>

static int fallos = 0;

static void check(int cond, const char *que) {
    if (cond) {
        printf("  ok    %s\n", que);
    } else {
        printf("  FALLO %s\n", que);
        fallos++;
    }
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    printf("== MiOS Fase 4: procesos y ejecucion de programas ==\n");

    /* --- fork + wait: el codigo de salida llega al padre --- */
    printf("[fork/wait]\n");
    {
        pid_t pid;
        pid_t visto;
        int status = 0;

        fflush(NULL);
        pid = fork();
        if (pid == 0) {
            _exit(7);
        }
        check(pid > 0, "fork devuelve el pid del hijo");
        visto = waitpid(pid, &status, 0);
        check(visto == pid, "waitpid devuelve ese mismo pid");
        check(WIFEXITED(status) && WEXITSTATUS(status) == 7,
              "el codigo de salida del hijo es 7");
    }

    /* --- exec de un programa real (ruta absoluta) --- */
    printf("[exec absoluto]\n");
    {
        pid_t pid;
        int status = 0;
        char *av[] = { (char *)"/hola", (char *)0 };

        fflush(NULL);
        pid = fork();
        if (pid == 0) {
            execv("/hola", av);
            _exit(127);
        }
        waitpid(pid, &status, 0);
        check(WIFEXITED(status) && WEXITSTATUS(status) == 0,
              "execv(/hola) termina con codigo 0");
    }

    /* --- execvp con busqueda en PATH y argumentos --- */
    printf("[execvp + PATH]\n");
    {
        pid_t pid;
        int status = 0;
        char *av[] = { (char *)"args", (char *)"uno", (char *)"dos", (char *)0 };

        check(setenv("PATH", "/", 1) == 0, "setenv PATH=/");
        fflush(NULL);
        pid = fork();
        if (pid == 0) {
            execvp("args", av);
            _exit(127);
        }
        waitpid(pid, &status, 0);
        check(WIFEXITED(status) && WEXITSTATUS(status) == 3,
              "execvp(args uno dos) sale con codigo = argc (3)");
    }

    /* --- pipe entre padre e hijo --- */
    printf("[pipe]\n");
    {
        int fds[2];
        pid_t pid;
        char buf[64];
        ssize_t n;

        check(pipe(fds) == 0, "pipe crea los dos descriptores");
        fflush(NULL);
        pid = fork();
        if (pid == 0) {
            const char msg[] = "mensaje-del-hijo";

            close(fds[0]);
            write(fds[1], msg, sizeof(msg));
            close(fds[1]);
            _exit(0);
        }
        close(fds[1]);
        buf[0] = '\0';
        n = read(fds[0], buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
        }
        close(fds[0]);
        waitpid(pid, (int *)0, 0);
        check(n > 0 && strcmp(buf, "mensaje-del-hijo") == 0,
              "el hijo escribe por el pipe y el padre lo lee");
    }

    /* --- ejecutar algo que no existe --- */
    printf("[exec fallido]\n");
    {
        pid_t pid;
        int status = 0;
        char *av[] = { (char *)"/no/existe/esto", (char *)0 };

        fflush(NULL);
        pid = fork();
        if (pid == 0) {
            execv("/no/existe/esto", av);
            _exit(127);
        }
        waitpid(pid, &status, 0);
        check(WIFEXITED(status) && WEXITSTATUS(status) == 127,
              "exec de algo inexistente deja codigo 127");
    }

    /* --- el hijo hereda los descriptores (su salida sale por stdout) --- */
    printf("[descriptores heredados]\n");
    {
        pid_t pid;
        int status = 0;
        char *av[] = { (char *)"/hola", (char *)0 };

        fflush(NULL);
        pid = fork();
        if (pid == 0) {
            execv("/hola", av);
            _exit(127);
        }
        waitpid(pid, &status, 0);
        check(WIFEXITED(status) && WEXITSTATUS(status) == 0,
              "el hijo escribe su salida por el mismo stdout");
    }

    printf("\n== resultado: %d fallo(s) ==\n", fallos);
    return fallos;
}
