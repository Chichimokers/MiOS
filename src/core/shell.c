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

    /* No es un comando interno: se intenta ejecutar como programa (de /bin,
     * via PATH).  Si no existe, el hijo lo avisa y devuelve 127. */
    prog_run(cmd, args);
}

/*
 * Ejecuta /etc/autorun al arrancar: una orden por linea.  Las lineas vacias y
 * las que empiezan por '#' se ignoran.  Sirve para lanzar programas o ajustar
 * cosas al arrancar sin teclear nada.
 */
static void shell_autorun(void) {
    char buf[4096];
    char *p;
    int fd;
    long n;

    fd = mios_open("/etc/autorun", MIOS_O_RDONLY, 0);
    if (fd < 0) {
        return;
    }
    n = mios_read(fd, buf, sizeof(buf) - 1);
    mios_close(fd);
    if (n <= 0) {
        return;
    }
    buf[n] = '\0';

    p = buf;
    while (*p != '\0') {
        char *line = p;
        char *e = p;
        char *q;
        mios_size_t len;

        while (*e != '\0' && *e != '\n') {
            e++;
        }
        if (*e == '\n') {
            *e = '\0';
            p = e + 1;
        } else {
            p = e;
        }

        len = mios_strlen(line);
        if (len > 0 && line[len - 1] == '\r') { /* ficheros de Windows */
            line[len - 1] = '\0';
        }

        q = line;
        while (*q == ' ' || *q == '\t') {
            q++;
        }
        if (*q == '\0' || *q == '#') {
            continue;
        }
        shell_execute(line);
    }
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

    shell_autorun();

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
