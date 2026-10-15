#include "mios.h"

/*
 * MiOS - carga de modulos del kernel.
 *
 * MiOS no tiene modprobe ni libc: abre cada .ko del initramfs y usa la
 * syscall finit_module. La ruta se construye con la version que reporta
 * uname(), asi que no hay que fijar la version a mano.
 *
 * Orden: las dependencias primero (mii antes que pcnet32). Ademas se dan
 * varias pasadas: si un modulo falla porque aun le falta una dependencia,
 * se reintenta en la siguiente pasada (equivale al orden de modprobe).
 */

struct module_entry {
    const char *relpath;
    const char *name;
};

static const struct module_entry boot_modules[] = {
    { "kernel/drivers/net/mii.ko",                          "mii" },
    { "kernel/drivers/net/ethernet/amd/pcnet32.ko",         "pcnet32" },
    { "kernel/drivers/net/ethernet/intel/e1000/e1000.ko",   "e1000" },
    { "kernel/drivers/net/ethernet/intel/e1000e/e1000e.ko", "e1000e" },
    { "kernel/drivers/net/vmxnet3/vmxnet3.ko",              "vmxnet3" },
    { "kernel/drivers/net/phy/realtek.ko",                  "realtek" },
    { "kernel/drivers/net/ethernet/realtek/r8169.ko",       "r8169" },
};

#define BOOT_MODULE_COUNT (sizeof(boot_modules) / sizeof(boot_modules[0]))
#define BOOT_MODULE_PASSES 3

/* estado por modulo: 0 pendiente, 1 cargado, 2 ya estaba, 3 no existe */
static int module_state[BOOT_MODULE_COUNT];
static int module_err[BOOT_MODULE_COUNT];

static mios_size_t append_str(char *buf, mios_size_t size, mios_size_t pos,
                              const char *s) {
    while (*s != '\0' && pos + 1 < size) {
        buf[pos] = *s;
        pos++;
        s++;
    }
    if (pos < size) {
        buf[pos] = '\0';
    }
    return pos;
}

static void build_path(char *out, mios_size_t size,
                       const char *base, const char *rel) {
    mios_size_t p = append_str(out, size, 0, base);
    append_str(out, size, p, rel);
}

void boot_load_modules(void) {
    struct mios_utsname u;
    char base[128];
    unsigned i;
    int pass;

    for (i = 0; i < BOOT_MODULE_COUNT; i++) {
        module_state[i] = MODULE_ESTADO_PENDIENTE;
        module_err[i] = 0;
    }

    if (mios_uname(&u) != 0) {
        cons_puts("modulos: no se pudo leer la version del kernel\n");
        return;
    }

    {
        mios_size_t n = append_str(base, sizeof(base), 0, "/lib/modules/");
        n = append_str(base, sizeof(base), n, u.release);
        append_str(base, sizeof(base), n, "/");
    }

    cons_puts("Cargando modulos de red...\n");

    for (pass = 0; pass < BOOT_MODULE_PASSES; pass++) {
        int progress = 0;

        for (i = 0; i < BOOT_MODULE_COUNT; i++) {
            const struct module_entry *mod = &boot_modules[i];
            char path[256];
            int fd;
            long r;

            if (module_state[i] != MODULE_ESTADO_PENDIENTE) {
                continue;
            }

            build_path(path, sizeof(path), base, mod->relpath);
            fd = mios_open(path, MIOS_O_RDONLY, 0);
            if (fd < 0) {
                cons_printf("  %-8s no encontrado (%s)\n",
                            mod->name, mios_strerror(mios_error(fd)));
                module_state[i] = MODULE_ESTADO_NO_EXISTE;
                continue;
            }

            r = mios_finit_module(fd, "", 0);
            mios_close(fd);

            if (r == 0) {
                module_state[i] = MODULE_ESTADO_CARGADO;
                cons_printf("  %-8s cargado\n", mod->name);
                progress = 1;
            } else if (mios_error(r) == MIOS_EEXIST) {
                module_state[i] = MODULE_ESTADO_YA_ESTABA;
                cons_printf("  %-8s ya estaba cargado\n", mod->name);
                progress = 1;
            } else {
                module_err[i] = mios_error(r);
            }
        }

        if (!progress) {
            break;
        }
    }

    for (i = 0; i < BOOT_MODULE_COUNT; i++) {
        if (module_state[i] == MODULE_ESTADO_PENDIENTE) {
            cons_printf("  %-8s error %d: %s\n",
                        boot_modules[i].name, module_err[i],
                        mios_strerror(module_err[i]));
            module_state[i] = MODULE_ESTADO_ERROR;
        }
    }

    cons_puts("");
}

int boot_module_total(void) {
    return (int)(sizeof(boot_modules) / sizeof(boot_modules[0]));
}

const char *boot_module_name(int index) {
    if (index < 0 || index >= boot_module_total()) {
        return "";
    }
    return boot_modules[index].name;
}

const char *boot_module_relpath(int index) {
    if (index < 0 || index >= boot_module_total()) {
        return "";
    }
    return boot_modules[index].relpath;
}

int boot_module_status(int index) {
    if (index < 0 || index >= boot_module_total()) {
        return MODULE_ESTADO_PENDIENTE;
    }
    return module_state[index];
}

int boot_module_err(int index) {
    if (index < 0 || index >= boot_module_total()) {
        return 0;
    }
    return module_err[index];
}
