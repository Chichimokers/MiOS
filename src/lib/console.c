#include "lib/lib.h"
#include "lib/format.h"
#include "kernel/syscall.h"

/*
 * MiOS - consola del sistema.
 *
 * Salida con buffer propio, historial circular y espejo por el puerto serie.
 * El formateo lo hace el motor comun (src/lib/format.c), el mismo que usan
 * printf/snprintf; aqui solo se le da una funcion que entrega los caracteres.
 *
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

/* Emisor para el motor de formato: cada caracter va a la consola. */
static void console_put(void *ctx, char c) {
    (void)ctx;
    cons_putc(c);
}

int cons_vprintf(const char *fmt, __builtin_va_list ap) {
    return mios_vformat(console_put, MIOS_NULL, fmt, ap);
}

void cons_printf(const char *fmt, ...) {
    __builtin_va_list ap;

    __builtin_va_start(ap, fmt);
    cons_vprintf(fmt, ap);
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
