#include "lib/lib.h"
#include "lib/format.h"

/*
 * MiOS - motor de formato (%d %i %u %x %X %o %c %s %p %f %e %g %n ...).
 *
 * Escribe caracter a caracter con el emisor que le pasen:
 *   - la consola usa cons_putc (buffer propio + espejo por el puerto serie);
 *   - snprintf usa un buffer.
 *
 * Soporta banderas (- + espacio 0 #), ancho, precision (.N) y los modificadores
 * de longitud hh h l ll z j t L.  Sin malloc: los numeros se formatean en una
 * pila de 80 bytes.
 */

typedef struct {
    mios_put_fn put;
    void       *ctx;
    mios_size_t count;
} out_t;

static void emit(out_t *o, char c) {
    o->put(o->ctx, c);
    o->count++;
}

static void emit_pad(out_t *o, char c, int n) {
    while (n-- > 0) {
        emit(o, c);
    }
}

static void emit_str(out_t *o, const char *s, int n) {
    while (n-- > 0) {
        emit(o, *s++);
    }
}

/* ------------------------------------------------------------------ */
/* Enteros                                                             */
/* ------------------------------------------------------------------ */

static void fmt_number(out_t *o, unsigned long v, int base, int upper,
                       int width, int zero, int left, int alt, int prec,
                       char sign) {
    char tmp[80];
    char pfx[2];
    int len = 0;
    int prefix = 0;
    int total, pad, i;
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";

    if (!(prec == 0 && v == 0)) {
        do {
            tmp[len] = digits[v % (unsigned long)base];
            len++;
            v /= (unsigned long)base;
        } while (v > 0);
    }

    if (alt && base == 16) {
        pfx[0] = '0';
        pfx[1] = upper ? 'X' : 'x';
        prefix = 2;
    } else if (alt && base == 8 && (len == 0 || tmp[len - 1] != '0')) {
        pfx[0] = '0';
        prefix = 1;
    }

    while (len < prec) {
        tmp[len] = '0';
        len++;
    }

    total = len + prefix + (sign != '\0' ? 1 : 0);
    pad = (width > total) ? width - total : 0;

    if (!left && !(zero && prec < 0)) {
        emit_pad(o, ' ', pad);
    }
    if (sign != '\0') {
        emit(o, sign);
    }
    for (i = 0; i < prefix; i++) {
        emit(o, pfx[i]);
    }
    if (!left && zero && prec < 0) {
        emit_pad(o, '0', pad);
    }
    for (i = len - 1; i >= 0; i--) {
        emit(o, tmp[i]);
    }
    if (left) {
        emit_pad(o, ' ', pad);
    }
}

/* ------------------------------------------------------------------ */
/* Coma flotante                                                       */
/* ------------------------------------------------------------------ */

static int is_nan(double d) {
    return d != d;
}

static int is_inf(double d) {
    return !is_nan(d) && (d - d) != 0.0;
}

/* Convierte d >= 0 a digitos decimales con 'prec' decimales (estilo %f).
 * Devuelve el numero de digitos de la parte entera; escribe en tmp. */
static int dtoa_fixed(double d, int prec, char *tmp, int tmax) {
    unsigned long long scale = 1;
    unsigned long long v;
    unsigned long long ip, fp;
    int i, len = 0;
    char rev[32];
    int rlen = 0;

    for (i = 0; i < prec; i++) {
        scale *= 10ULL;
    }

    /* Redondeo al ultimo decimal pedido. */
    v = (unsigned long long)(d * (double)scale + 0.5);

    ip = v / scale;
    fp = v % scale;

    if (ip == 0) {
        tmp[len++] = '0';
    } else {
        while (ip > 0 && rlen < (int)sizeof(rev)) {
            rev[rlen] = (char)('0' + (int)(ip % 10ULL));
            rlen++;
            ip /= 10ULL;
        }
        for (i = rlen - 1; i >= 0; i--) {
            if (len < tmax) {
                tmp[len] = rev[i];
                len++;
            }
        }
    }

    if (prec > 0) {
        char fr[24];
        if (prec > (int)sizeof(fr)) {
            prec = (int)sizeof(fr);
        }
        for (i = prec - 1; i >= 0; i--) {
            fr[i] = (char)('0' + (int)(fp % 10ULL));
            fp /= 10ULL;
        }
        if (len < tmax) {
            tmp[len++] = '.';
        }
        for (i = 0; i < prec; i++) {
            if (len < tmax) {
                tmp[len] = fr[i];
                len++;
            }
        }
    }
    tmp[len] = '\0';
    return len;
}

static void fmt_double(out_t *o, double d, int prec, int style, int upper,
                       int width, int zero, int left, int alt, int plus,
                       int space) {
    char buf[96];
    char sign = '\0';
    int len, pad, i;
    int exp = 0;

    (void)alt;

    if (is_nan(d)) {
        const char *s = upper ? "NAN" : "nan";
        len = 3;
        emit_pad(o, ' ', (width > len) ? width - len : 0);
        emit_str(o, s, len);
        return;
    }

    if (d < 0) {
        sign = '-';
        d = -d;
    } else if (plus) {
        sign = '+';
    } else if (space) {
        sign = ' ';
    }

    if (is_inf(d)) {
        const char *s = upper ? "INF" : "inf";
        len = 3;
        pad = (width > len + (sign ? 1 : 0)) ? width - len - (sign ? 1 : 0) : 0;
        if (!left) {
            emit_pad(o, ' ', pad);
        }
        if (sign) {
            emit(o, sign);
        }
        emit_str(o, s, len);
        if (left) {
            emit_pad(o, ' ', pad);
        }
        return;
    }

    if (prec < 0) {
        prec = 6;
    }
    if (prec > 17) {
        prec = 17;
    }

    if (style == 'e' || style == 'E' || style == 'g' || style == 'G') {
        /* Normaliza a [1,10). */
        if (d != 0.0) {
            while (d >= 10.0) {
                d /= 10.0;
                exp++;
            }
            while (d < 1.0) {
                d *= 10.0;
                exp--;
            }
        }
        if (style == 'g' || style == 'G') {
            prec = (prec == 0) ? 1 : prec - 1;
        }
        len = dtoa_fixed(d, prec, buf, (int)sizeof(buf) - 8);
        {
            char eb[16];
            int el = 0;
            int e = exp;
            int eneg = 0;
            char er[8];
            int rl = 0;
            if (e < 0) {
                eneg = 1;
                e = -e;
            }
            do {
                er[rl] = (char)('0' + (e % 10));
                rl++;
                e /= 10;
            } while (e > 0);
            eb[el++] = upper ? 'E' : 'e';
            eb[el++] = eneg ? '-' : '+';
            while (rl < 2) {
                er[rl] = '0';
                rl++;
            }
            while (rl > 0) {
                eb[el++] = er[--rl];
            }
            for (i = 0; i < el; i++) {
                buf[len + i] = eb[i];
            }
            len += el;
            buf[len] = '\0';
        }
    } else {
        len = dtoa_fixed(d, prec, buf, (int)sizeof(buf) - 1);
    }

    pad = (width > len + (sign ? 1 : 0)) ? width - len - (sign ? 1 : 0) : 0;
    if (!left && !zero) {
        emit_pad(o, ' ', pad);
    }
    if (sign) {
        emit(o, sign);
    }
    if (!left && zero) {
        emit_pad(o, '0', pad);
    }
    emit_str(o, buf, len);
    if (left) {
        emit_pad(o, ' ', pad);
    }
}

/* ------------------------------------------------------------------ */
/* Motor                                                               */
/* ------------------------------------------------------------------ */

int mios_vformat(mios_put_fn put, void *ctx, const char *fmt,
                 __builtin_va_list ap) {
    out_t o;

    o.put = put;
    o.ctx = ctx;
    o.count = 0;

    while (*fmt != '\0') {
        int left = 0, zero = 0, plus = 0, space = 0, alt = 0;
        int width = 0, prec = -1;
        int lenmod = 0; /* 0=int, 1=long, 2=long long, 3=size_t/ssize_t */
        char c;

        c = *fmt;
        if (c != '%') {
            emit(&o, c);
            fmt++;
            continue;
        }

        fmt++;
        for (;;) {
            if (*fmt == '-') {
                left = 1;
            } else if (*fmt == '0') {
                zero = 1;
            } else if (*fmt == '+') {
                plus = 1;
            } else if (*fmt == ' ') {
                space = 1;
            } else if (*fmt == '#') {
                alt = 1;
            } else {
                break;
            }
            fmt++;
        }
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }
        if (*fmt == '.') {
            fmt++;
            prec = 0;
            while (*fmt >= '0' && *fmt <= '9') {
                prec = prec * 10 + (*fmt - '0');
                fmt++;
            }
        }
        for (;;) {
            if (*fmt == 'l') {
                lenmod = (lenmod == 1) ? 2 : 1;
            } else if (*fmt == 'h') {
                lenmod = 0;
            } else if (*fmt == 'z' || *fmt == 'j' || *fmt == 't') {
                lenmod = 3;
            } else if (*fmt == 'L' || *fmt == 'q') {
                lenmod = 4;
            } else {
                break;
            }
            fmt++;
        }

        switch (*fmt) {
        case 'd':
        case 'i': {
            long v = (lenmod == 2) ? __builtin_va_arg(ap, long long)
                     : (lenmod >= 1) ? __builtin_va_arg(ap, long)
                     : (long)__builtin_va_arg(ap, int);
            char sg = '\0';
            if (v < 0) {
                sg = '-';
                fmt_number(&o, (unsigned long)(-(v + 1)) + 1UL, 10, 0, width,
                           zero, left, 0, prec, sg);
            } else {
                sg = plus ? '+' : (space ? ' ' : '\0');
                fmt_number(&o, (unsigned long)v, 10, 0, width, zero, left, 0,
                           prec, sg);
            }
            break;
        }
        case 'u': {
            unsigned long v = (lenmod == 2)
                                  ? __builtin_va_arg(ap, unsigned long long)
                              : (lenmod >= 1)
                                  ? __builtin_va_arg(ap, unsigned long)
                                  : (unsigned long)__builtin_va_arg(ap, unsigned int);
            fmt_number(&o, v, 10, 0, width, zero, left, 0, prec, '\0');
            break;
        }
        case 'x':
        case 'X': {
            unsigned long v = (lenmod == 2)
                                  ? __builtin_va_arg(ap, unsigned long long)
                              : (lenmod >= 1)
                                  ? __builtin_va_arg(ap, unsigned long)
                                  : (unsigned long)__builtin_va_arg(ap, unsigned int);
            fmt_number(&o, v, 16, *fmt == 'X', width, zero, left, alt, prec,
                       '\0');
            break;
        }
        case 'o': {
            unsigned long v = (lenmod >= 2)
                                  ? __builtin_va_arg(ap, unsigned long long)
                              : (lenmod >= 1)
                                  ? __builtin_va_arg(ap, unsigned long)
                                  : (unsigned long)__builtin_va_arg(ap, unsigned int);
            fmt_number(&o, v, 8, 0, width, zero, left, alt, prec, '\0');
            break;
        }
        case 'p': {
            void *p = __builtin_va_arg(ap, void *);
            if (p == (void *)0) {
                emit_pad(&o, ' ', (width > 5) ? width - 5 : 0);
                emit_str(&o, "(nil)", 5);
            } else {
                emit_str(&o, "0x", 2);
                fmt_number(&o, (unsigned long)p, 16, 0, width - 2, zero, left,
                           0, prec, '\0');
            }
            break;
        }
        case 'c': {
            char ch = (char)__builtin_va_arg(ap, int);
            emit_pad(&o, ' ', (width > 1) ? width - 1 : 0);
            emit(&o, ch);
            if (left) {
                emit_pad(&o, ' ', (width > 1) ? width - 1 : 0);
            }
            break;
        }
        case 's': {
            const char *s = __builtin_va_arg(ap, const char *);
            int n = 0;
            int pad;
            if (s == (const char *)0) {
                s = "(null)";
            }
            while (s[n] != '\0' && (prec < 0 || n < prec)) {
                n++;
            }
            pad = (width > n) ? width - n : 0;
            if (!left) {
                emit_pad(&o, ' ', pad);
            }
            emit_str(&o, s, n);
            if (left) {
                emit_pad(&o, ' ', pad);
            }
            break;
        }
        case 'f':
        case 'F':
        case 'e':
        case 'E':
        case 'g':
        case 'G': {
            double d = __builtin_va_arg(ap, double);
            fmt_double(&o, d, prec, (char)((*fmt == 'F') ? 'f' : *fmt),
                       (*fmt >= 'A' && *fmt <= 'Z'), width, zero, left, alt,
                       plus, space);
            break;
        }
        case '%': {
            emit(&o, '%');
            break;
        }
        case '\0': {
            emit(&o, '%');
            fmt--;
            break;
        }
        default: {
            emit(&o, '%');
            emit(&o, *fmt);
            break;
        }
        }
        fmt++;
    }

    return (int)o.count;
}

/* ------------------------------------------------------------------ */
/* snprintf                                                            */
/* ------------------------------------------------------------------ */

typedef struct {
    char       *buf;
    mios_size_t size;   /* tamano total, incluido el '\0' */
    mios_size_t pos;
} sbuf_t;

static void sbuf_put(void *ctx, char c) {
    sbuf_t *s = (sbuf_t *)ctx;

    if (s->pos + 1 < s->size) {
        s->buf[s->pos] = c;
    }
    s->pos++;
}

int mios_vsnprintf(char *buf, mios_size_t size, const char *fmt,
                   __builtin_va_list ap) {
    sbuf_t s;

    s.buf = buf;
    s.size = size;
    s.pos = 0;

    mios_vformat(sbuf_put, &s, fmt, ap);

    if (size > 0) {
        mios_size_t end = (s.pos < size - 1) ? s.pos : size - 1;
        buf[end] = '\0';
    }
    return (int)s.pos;
}
