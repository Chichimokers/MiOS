#include "mios.h"

/*
 * MiOS - consola del sistema.
 * Salida con buffer propio y un printf minimo (%s %c %d %u %x con ancho).
 * La entrada lee linea a linea desde el descriptor 0 (/dev/console).
 */

#define CONSOLE_BUF_SIZE 256

static char out_buffer[CONSOLE_BUF_SIZE];
static mios_size_t out_length = 0;

/* Historial circular de todo lo impreso (para el comando 'log'). */
#define CONSOLE_LOG_SIZE 65536

static char log_buffer[CONSOLE_LOG_SIZE];
static mios_size_t log_total = 0;
static mios_size_t log_pos = 0;
static int log_capture = 1;

/* Espejo por el puerto serie (si existe /dev/ttyS0). */
static int serial_fd = -1;
static int serial_tried = 0;

static void log_store(const char *s, mios_size_t n) {
    mios_size_t i;
    for (i = 0; i < n; i++) {
        log_buffer[log_pos] = s[i];
        log_pos++;
        if (log_pos >= CONSOLE_LOG_SIZE) {
            log_pos = 0;
        }
        if (log_total < CONSOLE_LOG_SIZE) {
            log_total++;
        }
    }
}

static void serial_mirror(const char *s, mios_size_t n) {
    if (serial_fd < 0 && !serial_tried) {
        serial_tried = 1;
        serial_fd = mios_open("/dev/ttyS0", MIOS_O_WRONLY, 0);
    }
    if (serial_fd >= 0) {
        mios_write(serial_fd, s, n);
    }
}

static void console_flush(void) {
    if (out_length > 0) {
        if (log_capture) {
            log_store(out_buffer, out_length);
        }
        mios_write(1, out_buffer, out_length);
        serial_mirror(out_buffer, out_length);
        out_length = 0;
    }
}

void cons_flush(void) {
    console_flush();
}

void cons_log_dump(void) {
    mios_size_t i;

    log_capture = 0;
    cons_puts("\n===== LOG DE LA CONSOLA =====\n");
    if (log_total < CONSOLE_LOG_SIZE) {
        for (i = 0; i < log_total; i++) {
            cons_putc(log_buffer[i]);
        }
    } else {
        for (i = 0; i < CONSOLE_LOG_SIZE; i++) {
            cons_putc(log_buffer[(log_pos + i) % CONSOLE_LOG_SIZE]);
        }
    }
    cons_puts("\n=============================\n\n");
    cons_flush();
    log_capture = 1;
}

void cons_putc(char c) {
    if (out_length >= CONSOLE_BUF_SIZE) {
        console_flush();
    }
    out_buffer[out_length] = c;
    out_length++;
    if (c == '\n') {
        console_flush();
    }
}

void cons_puts(const char *s) {
    while (*s != '\0') {
        cons_putc(*s);
        s++;
    }
}

static void emit_number(unsigned long v, int negative, int base,
                        int width, int zero, int left) {
    char digits[64];
    int len = 0;
    int i;

    do {
        int d = (int)(v % (unsigned long)base);
        digits[len] = (char)(d < 10 ? ('0' + d) : ('a' + (d - 10)));
        len++;
        v /= (unsigned long)base;
    } while (v > 0);

    if (negative) {
        digits[len] = '-';
        len++;
    }

    if (left) {
        for (i = len - 1; i >= 0; i--) {
            cons_putc(digits[i]);
        }
        for (i = len; i < width; i++) {
            cons_putc(' ');
        }
    } else if (zero && negative) {
        cons_putc('-');
        for (i = len; i < width; i++) {
            cons_putc('0');
        }
        for (i = len - 2; i >= 0; i--) {
            cons_putc(digits[i]);
        }
    } else {
        for (i = len; i < width; i++) {
            cons_putc(zero ? '0' : ' ');
        }
        for (i = len - 1; i >= 0; i--) {
            cons_putc(digits[i]);
        }
    }
}

void cons_printf(const char *fmt, ...) {
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);

    while (*fmt != '\0') {
        int left = 0;
        int zero = 0;
        int width = 0;
        int is_long = 0;
        char c;

        c = *fmt;
        if (c != '%') {
            cons_putc(c);
            fmt++;
            continue;
        }

        fmt++;
        while (*fmt == '-' || *fmt == '0') {
            if (*fmt == '-') {
                left = 1;
            } else {
                zero = 1;
            }
            fmt++;
        }
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }
        while (*fmt == 'l' || *fmt == 'z') {
            is_long = 1;
            fmt++;
        }

        switch (*fmt) {
        case 's': {
            const char *s = __builtin_va_arg(ap, const char *);
            mios_size_t len;
            mios_size_t k;

            if (s == MIOS_NULL) {
                s = "(null)";
            }
            len = mios_strlen(s);
            if (!left && width > (int)len) {
                for (k = len; k < (mios_size_t)width; k++) {
                    cons_putc(' ');
                }
            }
            for (k = 0; k < len; k++) {
                cons_putc(s[k]);
            }
            if (left && width > (int)len) {
                for (k = len; k < (mios_size_t)width; k++) {
                    cons_putc(' ');
                }
            }
            break;
        }
        case 'c': {
            int ch = __builtin_va_arg(ap, int);
            cons_putc((char)ch);
            break;
        }
        case 'd': {
            long v = is_long ? __builtin_va_arg(ap, long)
                             : (long)__builtin_va_arg(ap, int);
            if (v < 0) {
                emit_number((unsigned long)(-v), 1, 10, width, zero, left);
            } else {
                emit_number((unsigned long)v, 0, 10, width, zero, left);
            }
            break;
        }
        case 'u': {
            unsigned long v =
                is_long ? __builtin_va_arg(ap, unsigned long)
                        : (unsigned long)__builtin_va_arg(ap, unsigned int);
            emit_number(v, 0, 10, width, zero, left);
            break;
        }
        case 'x': {
            unsigned long v =
                is_long ? __builtin_va_arg(ap, unsigned long)
                        : (unsigned long)__builtin_va_arg(ap, unsigned int);
            emit_number(v, 0, 16, width, zero, left);
            break;
        }
        case '%': {
            cons_putc('%');
            break;
        }
        case '\0': {
            cons_putc('%');
            fmt--;
            break;
        }
        default: {
            cons_putc('%');
            cons_putc(*fmt);
            break;
        }
        }
        fmt++;
    }

    __builtin_va_end(ap);
}

int cons_read_line(char *buf, mios_size_t size) {
    mios_size_t n = 0;

    for (;;) {
        char c;
        long r = mios_read(0, &c, 1);

        if (r == 1) {
            if (c == '\n' || c == '\r') {
                break;
            }
            if (n + 1 < size) {
                buf[n] = c;
                n++;
            }
            continue;
        }
        if (r == 0) {
            break;
        }
        if (r == -MIOS_EPERM) {
            mios_sleep(1);
            continue;
        }
        if (r == -MIOS_EINTR) {
            continue;
        }
        mios_sleep(1);
    }

    buf[n] = '\0';
    return (n > 0) ? 1 : 0;
}
