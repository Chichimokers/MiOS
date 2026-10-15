#include "mios.h"

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

static const command_t shell_commands[] = {
    { "help",  "Muestra los comandos",             cmd_help },
    { "clear", "Limpia la pantalla",               cmd_clear },
    { "echo",  "Repite el texto indicado",         cmd_echo },
    { "exit",  "Avisa de que el PID 1 no termina", cmd_exit },
};

void cmd_shell_register(void) {
    shell_register(shell_commands,
                   sizeof(shell_commands) / sizeof(shell_commands[0]));
}
