#include "lib/lib.h"
#include "lib/ret.h"
#include "kernel/syscall.h"
#include "../../include/mios.h"
#include "../../include/sys/wait.h"

/*
 * MiOS - procesos: fork/exec/wait y senales.
 *
 * Nota sobre senales: en x86_64 el kernel exige el indicador SA_RESTORER, asi
 * que se le pasa un trozo de codigo propio que hace rt_sigreturn.
 */

/* ------------------------------------------------------------------ */
/* Procesos                                                            */
/* ------------------------------------------------------------------ */

pid_t fork(void) {
    return (pid_t)mios_ret(mios_fork());
}

int execve(const char *path, char *const argv[], char *const envp[]) {
    return (int)mios_ret(mios_execve(path, argv, envp));
}

int execv(const char *path, char *const argv[]) {
    return execve(path, argv, environ);
}

int execvp(const char *file, char *const argv[]) {
    const char *path;
    char full[512];
    size_t i, j;

    if (strchr(file, '/') != (char *)0) {
        return execv(file, argv);
    }

    path = getenv("PATH");
    if (path == (const char *)0) {
        path = "/bin:/usr/bin";
    }

    while (*path != '\0') {
        j = 0;
        while (path[j] != '\0' && path[j] != ':') {
            j++;
        }
        if (j + strlen(file) + 2 < sizeof(full)) {
            for (i = 0; i < j; i++) {
                full[i] = path[i];
            }
            if (j > 0 && full[j - 1] != '/') {
                full[j] = '/';
                j++;
            }
            strcpy(full + j, file);
            execv(full, argv);
        }
        path += j;
        if (*path == ':') {
            path++;
        }
    }

    errno = ENOENT;
    return -1;
}

pid_t wait(int *status) {
    return (pid_t)mios_ret(mios_wait4(-1, status, 0, (void *)0));
}

pid_t waitpid(pid_t pid, int *status, int options) {
    return (pid_t)mios_ret(mios_wait4(pid, status, options, (void *)0));
}

pid_t getppid(void) {
    return (pid_t)mios_getppid();
}

pid_t setsid(void) {
    return (pid_t)mios_ret(mios_setsid());
}

pid_t setpgid(pid_t pid, pid_t pgid) {
    return (pid_t)mios_ret(mios_setpgid(pid, pgid));
}

pid_t getpgid(pid_t pid) {
    return (pid_t)mios_ret(mios_getpgid(pid));
}

int kill(pid_t pid, int sig) {
    return (int)mios_ret(mios_kill(pid, sig));
}

int usleep(unsigned int usec) {
    return mios_sleep_ns((long)(usec / 1000000u),
                         (long)(usec % 1000000u) * 1000L);
}

/* ------------------------------------------------------------------ */
/* Senales                                                             */
/* ------------------------------------------------------------------ */

#define SA_RESTORER 0x04000000UL

struct kernel_sigaction {
    void (*handler)(int);
    unsigned long flags;
    void (*restorer)(void);
    unsigned long mask;
};

/* El kernel vuelve aqui tras ejecutar el manejador. */
static void mios_sig_restorer(void) {
    __asm__ volatile("mov $15, %%rax\n\t"
                     "syscall\n\t" ::: "rcx", "r11", "memory");
}

sighandler_t signal(int sig, sighandler_t handler) {
    struct kernel_sigaction sa;
    struct kernel_sigaction old;
    int r;

    sa.handler = handler;
    sa.flags = SA_RESTORER;
    sa.restorer = mios_sig_restorer;
    sa.mask = 0;

    r = mios_sigaction(sig, &sa, &old);
    if (r < 0) {
        return SIG_ERR;
    }
    return old.handler;
}

int raise(int sig) {
    return kill(getpid(), sig);
}

/* ------------------------------------------------------------------ */
/* Conjuntos de senales (API POSIX; Fase 7: la pide libiberty)         */
/* ------------------------------------------------------------------ */

#define MIOS_NSIG_MAX 64

static int sig_valido(int sig) {
    return sig >= 1 && sig <= MIOS_NSIG_MAX;
}

int sigemptyset(sigset_t *set) {
    int i;

    if (set == MIOS_NULL) {
        return -1;
    }
    for (i = 0; i < 16; i++) {
        set->__val[i] = 0;
    }
    return 0;
}

int sigfillset(sigset_t *set) {
    int i;

    if (set == MIOS_NULL) {
        return -1;
    }
    for (i = 0; i < 16; i++) {
        set->__val[i] = ~0UL;
    }
    set->__val[0] &= ~1UL;      /* la senal 0 no existe */
    return 0;
}

int sigaddset(sigset_t *set, int sig) {
    if (set == MIOS_NULL || !sig_valido(sig)) {
        return -1;
    }
    set->__val[0] |= 1UL << (sig - 1);
    return 0;
}

int sigdelset(sigset_t *set, int sig) {
    if (set == MIOS_NULL || !sig_valido(sig)) {
        return -1;
    }
    set->__val[0] &= ~(1UL << (sig - 1));
    return 0;
}

int sigismember(const sigset_t *set, int sig) {
    if (set == MIOS_NULL || !sig_valido(sig)) {
        return -1;
    }
    return (set->__val[0] >> (sig - 1)) & 1UL ? 1 : 0;
}

int sigprocmask(int how, const sigset_t *set, sigset_t *oldset) {
    int i;

    /* El kernel solo lee/escribe 8 bytes de estos conjuntos: se limpia el
     * resto para no dejar basura en la parte alta. */
    if (oldset != MIOS_NULL) {
        for (i = 0; i < 16; i++) {
            oldset->__val[i] = 0;
        }
    }
    return (int)mios_ret(mios_sigprocmask(how, set, oldset));
}
