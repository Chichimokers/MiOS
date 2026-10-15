#include "mios.h"

/*
 * MiOS - secuencia de arranque del PID 1.
 * Prepara /proc, /sys y /dev, y se asegura de que la consola existe.
 */

static void prepare_dir(const char *path) {
    mios_mkdir(path, 0755);
}

static void mount_fs(const char *source, const char *target, const char *type) {
    mios_mount(source, target, type, 0, MIOS_NULL);
}

static void ensure_console(void) {
    int fd = mios_open("/dev/console", MIOS_O_RDWR, 0);

    if (fd >= 0) {
        mios_close(fd);
        return;
    }

    if (mios_error(fd) == MIOS_ENOENT) {
        mios_mknod("/dev/console", MIOS_S_IFCHR | 0600, (5 << 8) | 1);
    }
}

void boot_setup_environment(void) {
    prepare_dir("/proc");
    prepare_dir("/sys");
    prepare_dir("/dev");

    mount_fs("proc", "/proc", "proc");
    mount_fs("sysfs", "/sys", "sysfs");
    mount_fs("devtmpfs", "/dev", "devtmpfs");

    ensure_console();
}

void boot_banner(void) {
    cons_puts("\n");
    cons_puts("================================\n");
    cons_puts("  ");
    cons_puts("MiOS ");
    cons_puts(MIOS_VERSION);
    cons_puts("\n");
    cons_puts("  Consola del sistema\n");
    cons_puts("================================\n");
    cons_puts("Escribe 'help' para ver los comandos.\n\n");
}
