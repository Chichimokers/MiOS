#ifndef MIOS_SYS_WAIT_H
#define MIOS_SYS_WAIT_H

/*
 * MiOS - sys/wait.h (cabecera publica).
 */

#include "types.h"

pid_t wait(int *status);
pid_t waitpid(pid_t pid, int *status, int options);

#define WNOHANG   1
#define WUNTRACED 2

#define WIFEXITED(s)   (((s) & 0x7f) == 0)
#define WEXITSTATUS(s) (((s) >> 8) & 0xff)
#define WIFSIGNALED(s) (((s) & 0x7f) != 0 && ((s) & 0x7f) != 0x7f)
#define WTERMSIG(s)    ((s) & 0x7f)
#define WIFSTOPPED(s)  (((s) & 0xff) == 0x7f)
#define WSTOPSIG(s)    (((s) >> 8) & 0xff)

#endif /* MIOS_SYS_WAIT_H */
