#ifndef MIOS_CORE_H
#define MIOS_CORE_H

#include "lib/types.h"

/*
 * MiOS - nucleo: arranque, shell y registro de comandos.
 */

typedef void (*cmd_func_t)(char *args);

typedef struct {
    const char *name;
    const char *desc;
    cmd_func_t func;
} command_t;

/* Shell (src/core/shell.c) */
void shell_register(const command_t *table, mios_size_t count);
mios_size_t shell_command_total(void);
const command_t *shell_command_get(mios_size_t index);
void shell_init(void);
void shell_run(void);

/* Arranque (src/core/boot.c) */
void boot_setup_environment(void);
void boot_banner(void);

/* Modulos del kernel (src/core/modules.c) */
enum {
    MODULE_ESTADO_PENDIENTE  = 0,
    MODULE_ESTADO_CARGADO    = 1,
    MODULE_ESTADO_YA_ESTABA  = 2,
    MODULE_ESTADO_NO_EXISTE  = 3,
    MODULE_ESTADO_ERROR      = 4,
};

void boot_load_modules(void);
int boot_module_total(void);
const char *boot_module_name(int index);
const char *boot_module_relpath(int index);
int boot_module_status(int index);
int boot_module_err(int index);

/* Punto de entrada (src/core/main.c) */
void mios_entry(void);

/* Registro de los modulos de comandos */
void cmd_shell_register(void);
void cmd_sys_register(void);
void cmd_hw_register(void);
void cmd_fs_register(void);
void cmd_net_register(void);

#endif /* MIOS_CORE_H */
