#include "mios.h"

/*
 * MiOS - comandos de informacion y control del sistema.
 */

static void cmd_info(char *args) {
    struct mios_utsname u;
    (void)args;

    cons_puts("\n===== MiOS =====\n");
    cons_printf("Version:       %s\n", MIOS_VERSION);

    if (mios_uname(&u) == 0) {
        cons_printf("Kernel:        %s\n", u.sysname);
        cons_printf("Release:       %s\n", u.release);
        cons_printf("Arquitectura:  %s\n", u.machine);
        cons_printf("Hostname:      %s\n", u.nodename);
    }

    cons_printf("PID de MiOS:   %d\n", mios_getpid());
    cons_printf("UID:           %d\n", mios_getuid());
    cons_printf("GID:           %d\n", mios_getgid());
    cons_puts("=================\n\n");
}

static void cmd_uname(char *args) {
    struct mios_utsname u;
    (void)args;

    if (mios_uname(&u) == 0) {
        cons_printf("%s %s %s %s\n",
                    u.sysname, u.release, u.machine, u.version);
    } else {
        cons_puts("uname: no disponible\n");
    }
}

static void cmd_uptime(char *args) {
    char buf[64];
    long n;
    long seconds;
    (void)args;

    n = file_read("/proc/uptime", buf, sizeof(buf));
    if (n < 0) {
        cons_printf("uptime: %s\n", mios_strerror(mios_error(n)));
        return;
    }

    seconds = mios_atol(buf);
    cons_printf("Encendido: %ld dias, %02d:%02d:%02d\n",
                seconds / 86400,
                (int)((seconds % 86400) / 3600),
                (int)((seconds % 3600) / 60),
                (int)(seconds % 60));
}

static void cmd_reboot(char *args) {
    (void)args;
    cons_puts("Reiniciando MiOS...\n");
    cons_flush();
    mios_reboot(0xfee1dead, 672274793, 0x01234567);
    cons_puts("reboot: el kernel rechazo la orden\n");
}

static void cmd_poweroff(char *args) {
    (void)args;
    cons_puts("Apagando MiOS...\n");
    cons_flush();
    mios_reboot(0xfee1dead, 672274793, 0x4321fedc);
    cons_puts("poweroff: el kernel rechazo la orden\n");
}

static char mods_buffer[65536];

static void print_module_line(const char *line, const char *end) {
    char name[64];
    char size[32];
    char used[16];
    const char *p = line;
    int i;

    if (line == end) {
        return;
    }

    i = 0;
    while (p < end && *p != ' ' && i < (int)sizeof(name) - 1) {
        name[i] = *p;
        i++;
        p++;
    }
    name[i] = '\0';

    while (p < end && *p == ' ') {
        p++;
    }
    i = 0;
    while (p < end && *p != ' ' && i < (int)sizeof(size) - 1) {
        size[i] = *p;
        i++;
        p++;
    }
    size[i] = '\0';

    while (p < end && *p == ' ') {
        p++;
    }
    i = 0;
    while (p < end && *p != ' ' && i < (int)sizeof(used) - 1) {
        used[i] = *p;
        i++;
        p++;
    }
    used[i] = '\0';

    cons_printf("  %-16s %10s bytes  usados=%s\n", name, size, used);
}

static void cmd_mods(char *args) {
    long n;
    const char *p;
    (void)args;

    n = file_read("/proc/modules", mods_buffer, sizeof(mods_buffer));
    if (n < 0) {
        cons_printf("mods: %s\n", mios_strerror(mios_error(n)));
        return;
    }

    cons_puts("\n===== MODULOS DEL KERNEL =====\n");
    p = mods_buffer;
    while (*p != '\0') {
        const char *end = p;
        while (*end != '\0' && *end != '\n') {
            end++;
        }
        print_module_line(p, end);
        p = (*end == '\n') ? end + 1 : end;
    }
    cons_puts("==============================\n\n");
}

static void cmd_loadmods(char *args) {
    (void)args;
    boot_load_modules();
}

static void cmd_loadmod(char *args) {
    char *tokens[1];
    int fd;
    long r;

    if (mios_split_words(args, tokens, 1) != 1) {
        cons_puts("Uso: loadmod <archivo.ko>\n");
        return;
    }

    fd = mios_open(tokens[0], MIOS_O_RDONLY, 0);
    if (fd < 0) {
        cons_printf("loadmod: %s: %s\n",
                    tokens[0], mios_strerror(mios_error(fd)));
        return;
    }

    r = mios_finit_module(fd, "", 0);
    mios_close(fd);

    if (r < 0) {
        cons_printf("loadmod: %s: error %d: %s\n",
                    tokens[0], mios_error(r), mios_strerror(mios_error(r)));
        return;
    }
    cons_printf("loadmod: %s cargado\n", tokens[0]);
}

static void cmd_klog(char *args) {
    char buf[4096];
    int fd;
    int guard = 0;
    (void)args;

    fd = mios_open("/dev/kmsg", MIOS_O_RDONLY | MIOS_O_NONBLOCK, 0);
    if (fd < 0) {
        cons_printf("klog: /dev/kmsg: %s\n",
                    mios_strerror(mios_error(fd)));
        return;
    }

    cons_puts("\n===== KLOG (/dev/kmsg) =====\n");
    while (guard < 500) {
        long n = mios_read(fd, buf, sizeof(buf) - 1);
        if (n <= 0) {
            break;
        }
        buf[n] = '\0';
        cons_puts(buf);
        guard++;
    }
    cons_puts("============================\n\n");
    mios_close(fd);
}

static void cmd_drivers(char *args) {
    int total;
    int i;
    int ok = 0;
    int fail = 0;
    (void)args;

    total = boot_module_total();
    cons_puts("\n===== DRIVERS DE RED =====\n");

    for (i = 0; i < total; i++) {
        const char *name = boot_module_name(i);
        int st = boot_module_status(i);

        switch (st) {
        case MODULE_ESTADO_CARGADO:
            cons_printf("  %-8s OK     cargado\n", name);
            ok++;
            break;
        case MODULE_ESTADO_YA_ESTABA:
            cons_printf("  %-8s OK     ya estaba cargado\n", name);
            ok++;
            break;
        case MODULE_ESTADO_NO_EXISTE:
            cons_printf("  %-8s FALLO  archivo no existe: %s\n",
                        name, boot_module_relpath(i));
            fail++;
            break;
        case MODULE_ESTADO_ERROR:
            cons_printf("  %-8s FALLO  error %d: %s\n",
                        name, boot_module_err(i),
                        mios_strerror(boot_module_err(i)));
            fail++;
            break;
        default:
            cons_printf("  %-8s pendiente\n", name);
            break;
        }
    }

    cons_printf("\nResumen: %d/%d cargados, %d fallos\n", ok, total, fail);
    cons_puts("==========================\n\n");
}

static void cmd_log(char *args) {
    (void)args;
    cons_log_dump();
}

static const command_t sys_commands[] = {
    { "info",     "Informacion del sistema",           cmd_info },
    { "uname",    "Informacion del kernel",            cmd_uname },
    { "uptime",   "Tiempo desde el arranque",          cmd_uptime },
    { "mods",     "Modulos del kernel cargados",       cmd_mods },
    { "loadmods", "Carga los modulos de red",          cmd_loadmods },
    { "loadmod",  "Carga un modulo: loadmod <ruta.ko>", cmd_loadmod },
    { "drivers",  "Estado de los drivers de red",       cmd_drivers },
    { "klog",     "Mensajes del kernel (/dev/kmsg)",   cmd_klog },
    { "log",      "Historial de la consola",            cmd_log },
    { "reboot",   "Reinicia la maquina",               cmd_reboot },
    { "poweroff", "Apaga la maquina",                  cmd_poweroff },
};

void cmd_sys_register(void) {
    shell_register(sys_commands,
                   sizeof(sys_commands) / sizeof(sys_commands[0]));
}
