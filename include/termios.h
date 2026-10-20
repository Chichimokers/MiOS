#ifndef MIOS_TERMIOS_H
#define MIOS_TERMIOS_H

/*
 * MiOS - termios.h (cabecera publica).
 *
 * La estructura es la del kernel (la que espera el ioctl TCGETS/TCSETS), no la
 * de glibc: aqui no hay campos de velocidad, porque la consola de MiOS no los
 * usa.  Lo que hace falta es lo de siempre: guardar la configuracion de la
 * consola, ponerla en modo crudo para leer teclas (cfmakeraw) y restaurarla.
 * Asi funciona el editor de texto de MiOS (userland/edit.c).
 */

#include "sys/types.h"

#define NCCS 19

typedef unsigned int  tcflag_t;
typedef unsigned char cc_t;
typedef unsigned int  speed_t;

struct termios {
    tcflag_t c_iflag;
    tcflag_t c_oflag;
    tcflag_t c_cflag;
    tcflag_t c_lflag;
    cc_t     c_line;
    cc_t     c_cc[NCCS];
};

struct winsize {
    unsigned short ws_row;
    unsigned short ws_col;
    unsigned short ws_xpixel;
    unsigned short ws_ypixel;
};

/* Acciones de tcsetattr. */
#define TCSANOW   0
#define TCSADRAIN 1
#define TCSAFLUSH 2

/* Peticiones de ioctl. */
#define TCGETS      0x5401
#define TCSETS      0x5402
#define TCSETSW     0x5403
#define TCSETSF     0x5404
#define TCSBRK      0x5409
#define TCFLSH      0x540B
#define TIOCGWINSZ  0x5413

/* Que encolar. */
#define TCIFLUSH  0
#define TCOFLUSH  1
#define TCIOFLUSH 2

/* Indices de c_cc. */
#define VINTR    0
#define VQUIT    1
#define VERASE   2
#define VKILL    3
#define VEOF     4
#define VTIME    5
#define VMIN     6
#define VSWTC    7
#define VSTART   8
#define VSTOP    9
#define VSUSP    10
#define VEOL     11
#define VREPRINT 12
#define VDISCARD 13
#define VWERASE  14
#define VLNEXT   15
#define VEOL2    16

/* c_iflag */
#define IGNBRK  0000001
#define BRKINT  0000002
#define IGNPAR  0000004
#define PARMRK  0000010
#define INPCK   0000020
#define ISTRIP  0000040
#define INLCR   0000100
#define IGNCR   0000200
#define ICRNL   0000400
#define IUCLC   0001000
#define IXON    0002000
#define IXANY   0004000
#define IXOFF   0010000
#define IMAXBEL 0020000
#define IUTF8   0040000

/* c_oflag */
#define OPOST   0000001
#define OLCUC   0000002
#define ONLCR   0000004
#define OCRNL   0000010
#define ONOCR   0000020
#define ONLRET  0000040
#define OFILL   0000100
#define OFDEL   0000200

/* c_cflag */
#define CSIZE   0000060
#define CS5     0000000
#define CS6     0000020
#define CS7     0000040
#define CS8     0000060
#define CSTOPB  0000100
#define CREAD   0000200
#define PARENB  0000400
#define PARODD  0001000
#define HUPCL   0002000
#define CLOCAL  0004000
#define B0      0000000
#define B50     0000001
#define B9600   0000015
#define B38400  0000017

/* c_lflag */
#define ISIG    0000001
#define ICANON  0000002
#define XCASE   0000004
#define ECHO    0000010
#define ECHOE   0000020
#define ECHOK   0000040
#define ECHONL  0000100
#define NOFLSH  0000200
#define TOSTOP  0000400
#define ECHOCTL 0001000
#define ECHOPRT 0002000
#define ECHOKE  0004000
#define FLUSHO  0010000
#define PENDIN  0040000
#define IEXTEN  0100000

int  tcgetattr(int fd, struct termios *t);
int  tcsetattr(int fd, int actions, const struct termios *t);
void cfmakeraw(struct termios *t);
int  tcflush(int fd, int queue);

#endif /* MIOS_TERMIOS_H */
