#include "mios.h"

/*
 * MiOS - comandos del sistema de archivos.
 */

static const char *fs_basename(const char *path) {
    const char *base = path;
    const char *p = path;
    while (*p != '\0') {
        if (*p == '/') {
            base = p + 1;
        }
        p++;
    }
    return base;
}

static int fs_is_dir(const char *path) {
    int fd;
    char tmp[64];
    long r;

    fd = mios_open(path, MIOS_O_RDONLY, 0);
    if (fd < 0) {
        return 0;
    }
    r = mios_getdents64(fd, tmp, sizeof(tmp));
    mios_close(fd);
    return r >= 0;
}

static void fs_build_path(char *out, mios_size_t size,
                          const char *dir, const char *name) {
    mios_size_t i = 0;
    const char *p;

    for (p = dir; *p != '\0' && i + 2 < size; p++) {
        out[i] = *p;
        i++;
    }
    if (i > 0 && out[i - 1] != '/' && i + 1 < size) {
        out[i] = '/';
        i++;
    }
    for (p = name; *p != '\0' && i + 1 < size; p++) {
        out[i] = *p;
        i++;
    }
    out[i] = '\0';
}

static void cmd_pwd(char *args) {
    char cwd[MIOS_MAX_PATH];
    long r;
    (void)args;

    r = mios_getcwd(cwd, sizeof(cwd));
    if (r > 0) {
        cons_printf("%s\n", cwd);
    } else {
        cons_puts("pwd: no se pudo obtener el directorio actual\n");
    }
}

static void cmd_ls(char *args) {
    char *tokens[1];
    const char *path = ".";
    char pathbuf[MIOS_MAX_PATH];
    int fd;
    char buf[2048];
    long nread;

    if (mios_split_words(args, tokens, 1) == 1) {
        mios_strncpy(pathbuf, tokens[0], sizeof(pathbuf) - 1);
        pathbuf[sizeof(pathbuf) - 1] = '\0';
        path = pathbuf;
    }

    fd = mios_open(path, MIOS_O_RDONLY, 0);
    if (fd < 0) {
        cons_printf("ls: %s: %s\n", path, mios_strerror(mios_error(fd)));
        return;
    }

    for (;;) {
        long pos = 0;
        nread = mios_getdents64(fd, buf, sizeof(buf));
        if (nread < 0) {
            cons_printf("ls: %s\n", mios_strerror(mios_error(nread)));
            break;
        }
        if (nread == 0) {
            break;
        }
        while (pos < nread) {
            struct mios_dirent64 *d = (struct mios_dirent64 *)(buf + pos);
            int hidden = 0;

            if (d->d_name[0] == '.') {
                if (d->d_name[1] == '\0') {
                    hidden = 1;
                } else if (d->d_name[1] == '.' && d->d_name[2] == '\0') {
                    hidden = 1;
                }
            }

            if (!hidden) {
                cons_puts(d->d_name);
                if (d->d_type == MIOS_DT_DIR) {
                    cons_putc('/');
                }
                cons_putc('\n');
            }

            pos += d->d_reclen;
        }
    }

    mios_close(fd);
}

static void cmd_cd(char *args) {
    char *tokens[1];
    const char *path = "/";
    long r;

    if (mios_split_words(args, tokens, 1) == 1) {
        path = tokens[0];
    }

    r = mios_chdir(path);
    if (r < 0) {
        cons_printf("cd: %s: %s\n", path, mios_strerror(mios_error(r)));
    }
}

static void cmd_cat(char *args) {
    char *tokens[1];

    if (mios_split_words(args, tokens, 1) != 1) {
        cons_puts("Uso: cat <archivo>\n");
        return;
    }

    file_cat(tokens[0]);
}

static void cmd_cp(char *args) {
    char *tokens[2];
    char dstbuf[MIOS_MAX_PATH];
    const char *src;
    const char *dst;
    long r;

    if (mios_split_words(args, tokens, 2) != 2) {
        cons_puts("Uso: cp <origen> <destino>\n");
        return;
    }

    src = tokens[0];
    dst = tokens[1];

    if (fs_is_dir(dst)) {
        fs_build_path(dstbuf, sizeof(dstbuf), dst, fs_basename(src));
        dst = dstbuf;
    }

    r = file_copy(src, dst);
    if (r < 0) {
        cons_printf("cp: %s -> %s: %s\n",
                    src, dst, mios_strerror(mios_error(r)));
        return;
    }

    cons_printf("Copiado: %s -> %s\n", src, dst);
}

static void cmd_mv(char *args) {
    char *tokens[2];
    char dstbuf[MIOS_MAX_PATH];
    const char *src;
    const char *dst;
    long r;

    if (mios_split_words(args, tokens, 2) != 2) {
        cons_puts("Uso: mv <origen> <destino>\n");
        return;
    }

    src = tokens[0];
    dst = tokens[1];

    if (fs_is_dir(dst)) {
        fs_build_path(dstbuf, sizeof(dstbuf), dst, fs_basename(src));
        dst = dstbuf;
    }

    r = mios_rename(src, dst);
    if (r == 0) {
        cons_printf("Movido: %s -> %s\n", src, dst);
        return;
    }

    if (mios_error(r) == MIOS_EXDEV) {
        long cr = file_copy(src, dst);
        if (cr == 0) {
            mios_unlink(src);
            cons_printf("Movido: %s -> %s\n", src, dst);
            return;
        }
        cons_printf("mv: %s -> %s: %s\n",
                    src, dst, mios_strerror(mios_error(cr)));
        return;
    }

    cons_printf("mv: %s -> %s: %s\n", src, dst, mios_strerror(mios_error(r)));
}

static void cmd_rm(char *args) {
    char *tokens[1];
    long r;

    if (mios_split_words(args, tokens, 1) != 1) {
        cons_puts("Uso: rm <archivo|directorio>\n");
        return;
    }

    r = mios_unlink(tokens[0]);
    if (r < 0 && mios_error(r) == MIOS_EISDIR) {
        r = mios_rmdir(tokens[0]);
    }

    if (r < 0) {
        cons_printf("rm: %s: %s\n", tokens[0], mios_strerror(mios_error(r)));
        return;
    }

    cons_printf("Eliminado: %s\n", tokens[0]);
}

static void cmd_mkdir(char *args) {
    char *tokens[1];
    long r;

    if (mios_split_words(args, tokens, 1) != 1) {
        cons_puts("Uso: mkdir <directorio>\n");
        return;
    }

    r = mios_mkdir(tokens[0], 0755);
    if (r < 0) {
        cons_printf("mkdir: %s: %s\n",
                    tokens[0], mios_strerror(mios_error(r)));
        return;
    }

    cons_printf("Creado: %s\n", tokens[0]);
}

static void cmd_rmdir(char *args) {
    char *tokens[1];
    long r;

    if (mios_split_words(args, tokens, 1) != 1) {
        cons_puts("Uso: rmdir <directorio>\n");
        return;
    }

    r = mios_rmdir(tokens[0]);
    if (r < 0) {
        cons_printf("rmdir: %s: %s\n",
                    tokens[0], mios_strerror(mios_error(r)));
        return;
    }

    cons_printf("Eliminado: %s\n", tokens[0]);
}

static void cmd_touch(char *args) {
    char *tokens[1];
    long r;

    if (mios_split_words(args, tokens, 1) != 1) {
        cons_puts("Uso: touch <archivo>\n");
        return;
    }

    r = mios_open(tokens[0],
                  MIOS_O_WRONLY | MIOS_O_CREAT | MIOS_O_APPEND, 0644);
    if (r < 0) {
        cons_printf("touch: %s: %s\n",
                    tokens[0], mios_strerror(mios_error(r)));
        return;
    }

    mios_close((int)r);
    cons_printf("Creado: %s\n", tokens[0]);
}

static const command_t fs_commands[] = {
    { "pwd",   "Directorio actual",            cmd_pwd },
    { "ls",    "Lista archivos",               cmd_ls },
    { "cd",    "Cambia de directorio",         cmd_cd },
    { "cat",   "Muestra un archivo",           cmd_cat },
    { "cp",    "Copia archivos",               cmd_cp },
    { "mv",    "Mueve o renombra archivos",    cmd_mv },
    { "rm",    "Elimina archivo o directorio", cmd_rm },
    { "mkdir", "Crea un directorio",           cmd_mkdir },
    { "rmdir", "Elimina un directorio",        cmd_rmdir },
    { "touch", "Crea un archivo vacio",        cmd_touch },
};

void cmd_fs_register(void) {
    shell_register(fs_commands, sizeof(fs_commands) / sizeof(fs_commands[0]));
}
