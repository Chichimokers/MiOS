#include "mios.h"
#include "../../include/unistd.h"
#include "../../include/errno.h"
#include "../../include/stdlib.h"

/*
 * MiOS - comandos basicos de la shell.
 */

static void cmd_help(char *args);

static void cmd_help(char *args) {
    mios_size_t i;
    mios_size_t total;
    (void)args;

    cons_puts("\nComandos disponibles:\n");
    total = shell_command_total();
    for (i = 0; i < total; i++) {
        const command_t *c = shell_command_get(i);
        cons_printf("  %-9s - %s\n", c->name, c->desc);
    }
    cons_puts("\n");
}

static void cmd_clear(char *args) {
    (void)args;
    cons_puts("\033[2J\033[H");
    cons_flush();
}

static void cmd_echo(char *args) {
    cons_puts(args);
    cons_putc('\n');
}

static void cmd_exit(char *args) {
    (void)args;
    cons_puts("MiOS: el PID 1 no puede terminar. El sistema sigue activo.\n");
}

static void cmd_cd(char *args) {
    const char *path = args;

    if (*path == '\0') {
        path = getenv("HOME");
        if (path == MIOS_NULL) {
            path = "/";
        }
    }
    if (chdir(path) != 0) {
        cons_printf("cd: %s: %s\n", path, strerror(errno));
    }
}

static void cmd_pwd(char *args) {
    char buf[MIOS_MAX_PATH];

    (void)args;
    if (getcwd(buf, sizeof(buf)) != MIOS_NULL) {
        cons_puts(buf);
        cons_putc('\n');
    }
}

static const command_t shell_commands[] = {
    { "help",  "Muestra los comandos",             cmd_help },
    { "clear", "Limpia la pantalla",               cmd_clear },
    { "echo",  "Repite el texto indicado",         cmd_echo },
    { "cd",    "Cambia el directorio (cd [dir])",  cmd_cd },
    { "pwd",   "Muestra el directorio actual",     cmd_pwd },
    { "exit",  "Avisa de que el PID 1 no termina", cmd_exit },
};

void cmd_shell_register(void) {
    shell_register(shell_commands,
                   sizeof(shell_commands) / sizeof(shell_commands[0]));
}
