/*
 * selftest: autoprueba de MiOS.
 *
 * Comprueba las piezas de la Fase 4 (procesos): fork/waitpid, execv/execvp
 * (con PATH y argumentos), tuberias y descriptores heredados.
 *
 * A proposito informa con cons_* (la consola de MiOS) en vez de printf: asi la
 * salida va a la pantalla *y* se duplica al puerto serie (src/lib/console.c),
 * de modo que se puede verificar desde /etc/autorun sin mirar la pantalla.
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <mios.h>

static int fallos;

static void check(const char *nombre, int cond) {
    if (!cond) {
        fallos++;
    }
    cons_printf("  %-34s %s\n", nombre, cond ? "OK" : "FALLO");
}

int main(void) {
    int st;
    pid_t pid;
    int fds[2];
    char buf[64];

    cons_puts("\nselftest de MiOS\n");

    /* Esta linea se escribe con el printf normal: va al stdout del programa
     * (fd 1 = /dev/console = pantalla VGA), no al puerto serie. */
    printf("(linea escrita con printf: solo en la pantalla)\n");
    fflush(stdout);

    /* 1. fork + waitpid: el hijo termina con codigo 7. */
    pid = fork();
    if (pid == 0) {
        _exit(7);
    }
    st = 0;
    check("fork + waitpid recoge el estado", pid > 0 && waitpid(pid, &st, 0) == pid);
    check("codigo de salida del hijo (7)", WIFEXITED(st) && WEXITSTATUS(st) == 7);

    /* 2. execv con ruta absoluta: /bin/hola termina con codigo 0. */
    pid = fork();
    if (pid == 0) {
        execv("/bin/hola", (char *[]){ "hola", (char *)0 });
        _exit(127);
    }
    st = 0;
    waitpid(pid, &st, 0);
    check("execv /bin/hola -> codigo 0", WIFEXITED(st) && WEXITSTATUS(st) == 0);

    /* 3. execvp buscando en PATH y pasando argumentos: args sale con argc. */
    pid = fork();
    if (pid == 0) {
        execvp("args", (char *[]){ "args", "uno", "dos", (char *)0 });
        _exit(127);
    }
    st = 0;
    waitpid(pid, &st, 0);
    check("execvp args (PATH) -> codigo 3", WIFEXITED(st) && WEXITSTATUS(st) == 3);

    /* 4. exec de algo que no existe: el hijo se queda con el codigo 127. */
    pid = fork();
    if (pid == 0) {
        execv("/no/existe", (char *[]){ "no", (char *)0 });
        _exit(127);
    }
    st = 0;
    waitpid(pid, &st, 0);
    check("execv de algo inexistente (127)", WIFEXITED(st) && WEXITSTATUS(st) == 127);

    /* 5. tuberia: el hijo escribe y el padre lo lee. */
    buf[0] = '\0';
    if (pipe(fds) == 0) {
        pid = fork();
        if (pid == 0) {
            close(fds[0]);
            write(fds[1], "hola tuberia", 12);
            close(fds[1]);
            _exit(0);
        }
        close(fds[1]);
        {
            long n = read(fds[0], buf, sizeof(buf) - 1);
            if (n > 0 && n < (long)sizeof(buf)) {
                buf[n] = '\0';
            }
        }
        close(fds[0]);
        st = 0;
        waitpid(pid, &st, 0);
        check("tuberia de hijo a padre", strcmp(buf, "hola tuberia") == 0);
    } else {
        check("tuberia de hijo a padre", 0);
    }

    /* 6. El hijo hereda la consola: lo que escribe el hijo llega al log serie. */
    pid = fork();
    if (pid == 0) {
        cons_puts("  (mensaje del hijo por la consola heredada)\n");
        cons_flush();
        _exit(0);
    }
    st = 0;
    waitpid(pid, &st, 0);
    check("el hijo hereda la consola", WIFEXITED(st) && WEXITSTATUS(st) == 0);

    /* 7. Identificadores de proceso. */
    check("getpid()/getppid()", getpid() > 1 && getppid() >= 1);

    cons_printf("resultado: %d fallo(s)\n\n", fallos);
    cons_flush();

    return fallos != 0;
}
