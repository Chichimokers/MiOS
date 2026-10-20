#ifndef MIOS_SIGNAL_H
#define MIOS_SIGNAL_H

/*
 * MiOS - signal.h (cabecera publica).
 * Solo el modelo clasico: signal(sig, manejador).
 */

typedef void (*sighandler_t)(int);

#define SIG_DFL ((sighandler_t)0)
#define SIG_IGN ((sighandler_t)1)
#define SIG_ERR ((sighandler_t)-1)

#define SIGHUP   1
#define SIGINT   2
#define SIGQUIT  3
#define SIGILL   4
#define SIGTRAP  5
#define SIGABRT  6
#define SIGBUS   7
#define SIGFPE   8
#define SIGKILL  9
#define SIGUSR1  10
#define SIGSEGV  11
#define SIGUSR2  12
#define SIGPIPE  13
#define SIGALRM  14
#define SIGTERM  15
#define SIGCHLD  17
#define SIGCONT  18
#define SIGSTOP  19
#define SIGTSTP  20

#define SIG_BLOCK   0
#define SIG_UNBLOCK 1
#define SIG_SETMASK 2

/*
 * Conjunto de senales (API POSIX, Fase 7: la pide libiberty).
 *
 * El kernel x86_64 usa 8 bytes (64 senales) en rt_sigprocmask, asi que solo
 * se usan los bits de __val[0]; el resto se queda a cero para que el tamano
 * coincida con el de glibc (128 bytes) y el codigo que reserva uno funcione
 * igual.
 */
#ifndef MIOS_SIGSET_T_DEFINED
#define MIOS_SIGSET_T_DEFINED
typedef struct {
    unsigned long __val[16];
} sigset_t;
#endif

int raise(int sig);
sighandler_t signal(int sig, sighandler_t handler);

int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
int sigaddset(sigset_t *set, int sig);
int sigdelset(sigset_t *set, int sig);
int sigismember(const sigset_t *set, int sig);
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);

#endif /* MIOS_SIGNAL_H */
