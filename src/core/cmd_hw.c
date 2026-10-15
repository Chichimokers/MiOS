#include "mios.h"

/*
 * MiOS - informacion de hardware leida de /proc.
 */

static char hw_buffer[65536];

static int line_prefix(const char *line, const char *prefix) {
    while (*prefix != '\0') {
        if (*line != *prefix) {
            return 0;
        }
        line++;
        prefix++;
    }
    return 1;
}

static const char *skip_to_value(const char *line) {
    while (*line != '\0' && *line != ':') {
        line++;
    }
    if (*line == ':') {
        line++;
    }
    while (*line == ' ' || *line == '\t') {
        line++;
    }
    return line;
}

static void print_line(const char *start, const char *end) {
    const char *p;
    for (p = start; p < end; p++) {
        cons_putc(*p);
    }
    cons_putc('\n');
}

static void cmd_hw(char *args) {
    struct mios_utsname u;
    long n;
    char model[64];
    int cores = 0;
    (void)args;

    cons_puts("\n===== ESTADO DEL HARDWARE =====\n");

    if (mios_uname(&u) == 0) {
        cons_printf("Kernel:        %s %s\n", u.sysname, u.release);
        cons_printf("Arquitectura:  %s\n", u.machine);
    }

    n = file_read("/proc/cpuinfo", hw_buffer, sizeof(hw_buffer));
    if (n > 0) {
        const char *p = hw_buffer;
        model[0] = '\0';
        while (*p != '\0') {
            const char *end = p;
            while (*end != '\0' && *end != '\n') {
                end++;
            }
            if (line_prefix(p, "processor")) {
                cores++;
            }
            if (line_prefix(p, "model name") && model[0] == '\0') {
                const char *v = skip_to_value(p);
                int i = 0;
                while (*v != '\0' && *v != '\n' && i < (int)sizeof(model) - 1) {
                    model[i] = *v;
                    i++;
                    v++;
                }
                while (i > 0 && (model[i - 1] == ' ' || model[i - 1] == '\t')) {
                    i--;
                }
                model[i] = '\0';
            }
            p = (*end == '\n') ? end + 1 : end;
        }
        if (model[0] != '\0') {
            cons_printf("CPU:           %s\n", model);
        }
        cons_printf("Nucleos:       %d\n", cores);
    } else {
        cons_puts("CPU:           (no se pudo leer /proc/cpuinfo)\n");
    }

    n = file_read("/proc/meminfo", hw_buffer, sizeof(hw_buffer));
    if (n > 0) {
        const char *p = hw_buffer;
        while (*p != '\0') {
            const char *end = p;
            while (*end != '\0' && *end != '\n') {
                end++;
            }
            if (line_prefix(p, "MemTotal")) {
                cons_puts("Memoria total: ");
                print_line(skip_to_value(p), end);
            } else if (line_prefix(p, "MemAvailable")) {
                cons_puts("Memoria libre: ");
                print_line(skip_to_value(p), end);
            }
            p = (*end == '\n') ? end + 1 : end;
        }
    }
    cons_puts("===============================\n\n");
}

static void uevent_value(const char *text, const char *key,
                         char *out, mios_size_t size) {
    const char *p = text;
    mios_size_t klen = mios_strlen(key);
    mios_size_t i;

    out[0] = '\0';
    while (*p != '\0') {
        const char *end = p;
        const char *v;
        while (*end != '\0' && *end != '\n') {
            end++;
        }
        if ((mios_size_t)(end - p) >= klen && mios_strncmp(p, key, klen) == 0) {
            v = p + klen;
            i = 0;
            while (v < end && i + 1 < size) {
                out[i] = *v;
                i++;
                v++;
            }
            out[i] = '\0';
            return;
        }
        p = (*end == '\n') ? end + 1 : end;
    }
}

static void pci_show_device(const char *addr) {
    char path[192];
    char id[32];
    char cls[32];
    char drv[32];
    const char *p;
    mios_size_t i = 0;
    long n;

    for (p = "/sys/bus/pci/devices/"; *p != '\0' && i + 1 < sizeof(path); p++) {
        path[i] = *p;
        i++;
    }
    for (p = addr; *p != '\0' && i + 1 < sizeof(path); p++) {
        path[i] = *p;
        i++;
    }
    for (p = "/uevent"; *p != '\0' && i + 1 < sizeof(path); p++) {
        path[i] = *p;
        i++;
    }
    path[i] = '\0';

    n = file_read(path, hw_buffer, sizeof(hw_buffer));
    if (n <= 0) {
        return;
    }

    uevent_value(hw_buffer, "PCI_ID=", id, sizeof(id));
    uevent_value(hw_buffer, "PCI_CLASS=", cls, sizeof(cls));
    uevent_value(hw_buffer, "DRIVER=", drv, sizeof(drv));

    cons_printf("  %-14s %-10s class=%-8s driver=%s\n",
                addr, id[0] != '\0' ? id : "-",
                cls[0] != '\0' ? cls : "-",
                drv[0] != '\0' ? drv : "(ninguno)");
}

static int pci_read_uevent(const char *addr, char *id, mios_size_t idsz,
                           char *cls, mios_size_t clssz,
                           char *drv, mios_size_t drvsz) {
    char path[192];
    const char *p;
    mios_size_t i = 0;
    long n;

    for (p = "/sys/bus/pci/devices/"; *p != '\0' && i + 1 < sizeof(path); p++) {
        path[i] = *p;
        i++;
    }
    for (p = addr; *p != '\0' && i + 1 < sizeof(path); p++) {
        path[i] = *p;
        i++;
    }
    for (p = "/uevent"; *p != '\0' && i + 1 < sizeof(path); p++) {
        path[i] = *p;
        i++;
    }
    path[i] = '\0';

    n = file_read(path, hw_buffer, sizeof(hw_buffer));
    if (n <= 0) {
        return 0;
    }
    uevent_value(hw_buffer, "PCI_ID=", id, idsz);
    uevent_value(hw_buffer, "PCI_CLASS=", cls, clssz);
    uevent_value(hw_buffer, "DRIVER=", drv, drvsz);
    return 1;
}

static void cmd_pci(char *args) {
    int fd;
    char buf[4096];
    long nread;
    (void)args;

    cons_puts("\n===== DISPOSITIVOS PCI =====\n");
    fd = mios_open("/sys/bus/pci/devices", MIOS_O_RDONLY, 0);
    if (fd < 0) {
        cons_printf("pci: /sys/bus/pci/devices: %s\n",
                    mios_strerror(mios_error(fd)));
        return;
    }
    for (;;) {
        long pos = 0;
        nread = mios_getdents64(fd, buf, sizeof(buf));
        if (nread <= 0) {
            break;
        }
        while (pos < nread) {
            struct mios_dirent64 *d = (struct mios_dirent64 *)(buf + pos);
            if (d->d_name[0] != '.') {
                pci_show_device(d->d_name);
            }
            pos += d->d_reclen;
        }
    }
    mios_close(fd);
    cons_puts("============================\n\n");
}

static void cmd_nic(char *args) {
    int fd;
    char buf[4096];
    long nread;
    int found = 0;
    (void)args;

    fd = mios_open("/sys/bus/pci/devices", MIOS_O_RDONLY, 0);
    if (fd < 0) {
        cons_printf("nic: /sys/bus/pci/devices: %s\n",
                    mios_strerror(mios_error(fd)));
        return;
    }
    for (;;) {
        long pos = 0;
        nread = mios_getdents64(fd, buf, sizeof(buf));
        if (nread <= 0) {
            break;
        }
        while (pos < nread) {
            struct mios_dirent64 *d = (struct mios_dirent64 *)(buf + pos);
            if (d->d_name[0] != '.') {
                char id[32];
                char cls[32];
                char drv[32];
                if (pci_read_uevent(d->d_name, id, sizeof(id),
                                    cls, sizeof(cls), drv, sizeof(drv))) {
                    if (cls[0] == '2') {
                        cons_printf("NIC %-14s %-10s class=%-8s driver=%s\n",
                                    d->d_name, id[0] != '\0' ? id : "-",
                                    cls, drv[0] != '\0' ? drv : "(ninguno)");
                        found = 1;
                    }
                }
            }
            pos += d->d_reclen;
        }
    }
    mios_close(fd);
    if (!found) {
        cons_puts("NIC: ningun dispositivo PCI de red (class=20000)\n");
    }
}

static const command_t hw_commands[] = {
    { "hw",  "Estado del hardware (CPU, RAM)", cmd_hw },
    { "pci", "Dispositivos PCI y su driver",   cmd_pci },
    { "nic", "Dispositivo PCI de red y driver", cmd_nic },
};

void cmd_hw_register(void) {
    shell_register(hw_commands, sizeof(hw_commands) / sizeof(hw_commands[0]));
}
