#include "lib/lib.h"
#include "lib/ret.h"
#include "kernel/syscall.h"
#include "../../include/termios.h"

/*
 * MiOS - termios (puerto serie y consola).
 *
 * Todo va por el mismo ioctl del kernel que ya usa isatty(); aqui solo se
 * envuelve con las funciones de siempre.  El caso que importa es cfmakeraw():
 * deja la consola en modo crudo (sin eco, sin canonico, sin Ctrl-C ni Ctrl-S
 * del kernel) para poder leer teclas una a una, que es lo que necesita el
 * editor de texto de MiOS (userland/edit.c).
 */

int tcgetattr(int fd, struct termios *t) {
    return (int)mios_ret(mios_ioctl(fd, TCGETS, (void *)t));
}

int tcsetattr(int fd, int actions, const struct termios *t) {
    unsigned long request;

    switch (actions) {
    case TCSADRAIN:
        request = TCSETSW;
        break;
    case TCSAFLUSH:
        request = TCSETSF;
        break;
    default:
        request = TCSETS;
        break;
    }
    return (int)mios_ret(mios_ioctl(fd, request, (void *)t));
}

void cfmakeraw(struct termios *t) {
    /* Nada de traduccion de entrada/salida ni de control de flujo. */
    t->c_iflag &= ~(tcflag_t)(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR |
                              IGNCR | ICRNL | IXON);
    t->c_oflag &= ~(tcflag_t)OPOST;
    /* Sin eco, sin modo canonico, sin senales (Ctrl-C llega como tecla). */
    t->c_lflag &= ~(tcflag_t)(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    t->c_cflag &= ~(tcflag_t)(CSIZE | PARENB);
    t->c_cflag |= (tcflag_t)CS8;
    /* Una tecla por lectura, sin esperas. */
    t->c_cc[VMIN] = 1;
    t->c_cc[VTIME] = 0;
}

int tcflush(int fd, int queue) {
    return (int)mios_ret(mios_ioctl(fd, TCFLSH, (void *)(long)queue));
}
