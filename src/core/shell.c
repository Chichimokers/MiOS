#include "mios.h"

/*
 * MiOS - shell: registro de comandos, parser y bucle principal.
 * Cada modulo de comandos se registra en shell_init().
 */

#define SHELL_MAX_COMMANDS 64

static const command_t *shell_commands[SHELL_MAX_COMMANDS];
static mios_size_t shell_command_count = 0;

void shell_register(const command_t *table, mios_size_t count) {
    mios_size_t i;
    for (i = 0; i < count && shell_command_count < SHELL_MAX_COMMANDS; i++) {
        shell_commands[shell_command_count] = &table[i];
        shell_command_count++;
    }
}

mios_size_t shell_command_total(void) {
    return shell_command_count;
}

const command_t *shell_command_get(mios_size_t index) {
    if (index >= shell_command_count) {
        return MIOS_NULL;
    }
    return shell_commands[index];
}

static void shell_split(char *line, char **cmd, char **args) {
    char *p = line;

    while (*p == ' ' || *p == '\t') {
        p++;
    }
    *cmd = p;
    while (*p != '\0' && *p != ' ' && *p != '\t') {
        p++;
    }
    if (*p != '\0') {
        *p = '\0';
        p++;
    }
    while (*p == ' ' || *p == '\t') {
        p++;
    }
    *args = p;
}

static void shell_execute(char *line) {
    char *cmd;
    char *args;
    mios_size_t i;

    shell_split(line, &cmd, &args);

    if (*cmd == '\0') {
        return;
    }

    for (i = 0; i < shell_command_count; i++) {
        if (mios_strcmp(cmd, shell_commands[i]->name) == 0) {
            shell_commands[i]->func(args);
            return;
        }
    }

    cons_printf("Comando desconocido: %s\n", cmd);
    cons_puts("Escribe 'help' para ver los comandos.\n");
}

void shell_init(void) {
    shell_command_count = 0;
    cmd_shell_register();
    cmd_sys_register();
    cmd_hw_register();
    cmd_fs_register();
    cmd_net_register();
}

void shell_run(void) {
    char line[MIOS_MAX_LINE];

    for (;;) {
        cons_puts("MiOS> ");
        cons_flush();
        if (!cons_read_line(line, sizeof(line))) {
            mios_sleep(1);
            continue;
        }
        shell_execute(line);
    }
}
