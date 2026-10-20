#include "lib/lib.h"
#include "../../include/mios.h"

/*
 * MiOS - scanf: analizador de entrada con formato.
 *
 * La fuente puede ser una cadena o un FILE; el analizador es el mismo.
 * Soporta: %d %i %u %o %x %X %p %c %s %e %f %g %n %% con ancho, supresion (*)
 * y los modificadores h hh l ll z. No hay %[ (conjuntos).
 */

typedef struct {
    const char *s;   /* modo cadena (NULL si es fichero) */
    FILE       *f;   /* modo fichero */
    int         pos;
    int         pushed; /* caracter devuelto por el analizador (0 = ninguno) */
    int         count;  /* caracteres consumidos (para %n) */
} src_t;

static int sc_get(src_t *sc) {
    int c;

    if (sc->pushed != 0) {
        c = sc->pushed - 1;
        sc->pushed = 0;
    } else if (sc->s != (const char *)0) {
        c = (unsigned char)sc->s[sc->pos];
        if (c == 0) {
            return -1;
        }
        sc->pos++;
    } else {
        c = fgetc(sc->f);
    }
    if (c >= 0) {
        sc->count++;
    }
    return c;
}

static void sc_unget(src_t *sc, int c) {
    if (c < 0) {
        return;
    }
    sc->pushed = c + 1;
    sc->count--;
}

static int sc_skip_ws(src_t *sc) {
    int c;

    do {
        c = sc_get(sc);
    } while (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' ||
             c == '\f');
    return c;
}

static int digit_val(int c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 10;
    }
    return -1;
}

static int store_int(long long v, int lenmod, int suppress, __builtin_va_list *ap) {
    if (suppress) {
        return 1;
    }
    switch (lenmod) {
    case 2:
        *__builtin_va_arg(*ap, long long *) = v;
        break;
    case 1:
        *__builtin_va_arg(*ap, long *) = (long)v;
        break;
    case 3:
        *__builtin_va_arg(*ap, long *) = (long)v;
        break;
    case 4: /* hh */
        *__builtin_va_arg(*ap, char *) = (char)v;
        break;
    case 5: /* h */
        *__builtin_va_arg(*ap, short *) = (short)v;
        break;
    default:
        *__builtin_va_arg(*ap, int *) = (int)v;
        break;
    }
    return 1;
}

static int store_uint(unsigned long long v, int lenmod, int suppress,
                      __builtin_va_list *ap) {
    if (suppress) {
        return 1;
    }
    switch (lenmod) {
    case 2:
        *__builtin_va_arg(*ap, unsigned long long *) = v;
        break;
    case 1:
    case 3:
        *__builtin_va_arg(*ap, unsigned long *) = (unsigned long)v;
        break;
    case 4:
        *__builtin_va_arg(*ap, unsigned char *) = (unsigned char)v;
        break;
    case 5:
        *__builtin_va_arg(*ap, unsigned short *) = (unsigned short)v;
        break;
    default:
        *__builtin_va_arg(*ap, unsigned int *) = (unsigned int)v;
        break;
    }
    return 1;
}

static int scan_int(src_t *sc, int base, int width, int *neg,
                    unsigned long long *out) {
    int c;
    int digits = 0;
    unsigned long long v = 0;

    *neg = 0;
    c = sc_skip_ws(sc);
    if (c < 0) {
        return 0;
    }
    if (c == '+' || c == '-') {
        *neg = (c == '-');
        c = sc_get(sc);
    }

    if (base == 0) {
        if (c == '0') {
            int d = sc_get(sc);
            if (d == 'x' || d == 'X') {
                int e = sc_get(sc);
                if (digit_val(e) >= 0 && digit_val(e) < 16) {
                    base = 16;
                    c = e;
                } else {
                    /* "0x" sin digito: el numero es solo el 0 */
                    sc_unget(sc, e);
                    sc_unget(sc, d);
                    *out = 0;
                    return 1;
                }
            } else {
                base = 8;
                sc_unget(sc, d);
            }
        } else {
            base = 10;
        }
    } else if (base == 16) {
        if (c == '0') {
            int d = sc_get(sc);
            if (d == 'x' || d == 'X') {
                c = sc_get(sc);
            } else {
                sc_unget(sc, d);
            }
        }
    }

    for (;;) {
        int d;
        if (width > 0 && digits >= width) {
            break;
        }
        d = digit_val(c);
        if (d < 0 || d >= base) {
            break;
        }
        v = v * (unsigned long long)base + (unsigned long long)d;
        digits++;
        c = sc_get(sc);
    }
    sc_unget(sc, c);
    *out = v;
    return digits > 0;
}

/* Lee un numero en coma flotante: entero, punto decimal y exponente. */
static int scan_double(src_t *sc, int width, double *out) {
    int c;
    int used = 0;
    int any = 0;
    double v = 0.0;
    int neg = 0;

    c = sc_skip_ws(sc);
    if (c < 0) {
        return 0;
    }
    if (c == '+' || c == '-') {
        neg = (c == '-');
        c = sc_get(sc);
        used++;
    }
    while (c >= '0' && c <= '9' && (width <= 0 || used < width)) {
        v = v * 10.0 + (double)(c - '0');
        any = 1;
        used++;
        c = sc_get(sc);
    }
    if (c == '.' && (width <= 0 || used < width)) {
        double scale = 0.1;
        c = sc_get(sc);
        used++;
        while (c >= '0' && c <= '9' && (width <= 0 || used < width)) {
            v += (double)(c - '0') * scale;
            scale *= 0.1;
            any = 1;
            used++;
            c = sc_get(sc);
        }
    }
    if (any && (c == 'e' || c == 'E') && (width <= 0 || used < width)) {
        int save_used = used;
        int eneg = 0;
        int e = 0;
        int eany = 0;
        int c2 = sc_get(sc);

        used++;
        if (c2 == '+' || c2 == '-') {
            eneg = (c2 == '-');
            c2 = sc_get(sc);
            used++;
        }
        while (c2 >= '0' && c2 <= '9' && (width <= 0 || used < width)) {
            e = e * 10 + (c2 - '0');
            if (e > 1000) {
                e = 1000;
            }
            eany = 1;
            used++;
            c2 = sc_get(sc);
        }
        if (eany) {
            while (e-- > 0) {
                v = eneg ? v / 10.0 : v * 10.0;
            }
            c = c2;
        } else {
            used = save_used;
            sc_unget(sc, c2);
        }
    }
    sc_unget(sc, c);
    if (!any) {
        return 0;
    }
    *out = neg ? -v : v;
    return 1;
}

static int scan_core(src_t *sc, const char *fmt, __builtin_va_list ap) {
    int assigned = 0;
    __builtin_va_list a; /* copia local: &ap no vale porque el parametro decae */

    __builtin_va_copy(a, ap);

    while (*fmt != '\0') {
        if (*fmt == ' ' || *fmt == '\t' || *fmt == '\n') {
            fmt++;
            continue;
        }
        if (*fmt != '%') {
            int c = sc_get(sc);
            if (c != *fmt) {
                sc_unget(sc, c);
                break;
            }
            fmt++;
            continue;
        }

        fmt++;
        if (*fmt == '%') {
            int c = sc_get(sc);
            if (c != '%') {
                sc_unget(sc, c);
                break;
            }
            fmt++;
            continue;
        }

        {
            int suppress = 0;
            int width = 0;
            int lenmod = 0;
            int conv;
            int done;

            if (*fmt == '*') {
                suppress = 1;
                fmt++;
            }
            while (*fmt >= '0' && *fmt <= '9') {
                width = width * 10 + (*fmt - '0');
                fmt++;
            }
            for (;;) {
                if (*fmt == 'l') {
                    lenmod = (lenmod == 1) ? 2 : 1;
                } else if (*fmt == 'h') {
                    lenmod = (lenmod == 5) ? 4 : 5;
                } else if (*fmt == 'z' || *fmt == 'j' || *fmt == 't') {
                    lenmod = 3;
                } else {
                    break;
                }
                fmt++;
            }

            conv = (unsigned char)*fmt;
            if (conv == '\0') {
                break;
            }

            switch (conv) {
            case 'd':
            case 'i':
            case 'u': {
                unsigned long long v = 0;
                int neg = 0;
                int base = (conv == 'i') ? 0 : 10;

                if (!scan_int(sc, base, width, &neg, &v)) {
                    return assigned;
                }
                done = 1;
                if (conv == 'u') {
                    store_uint(neg ? (0ULL - v) : v, lenmod, suppress, &a);
                } else {
                    store_int(neg ? -(long long)v : (long long)v, lenmod,
                              suppress, &a);
                }
                break;
            }
            case 'o':
            case 'x':
            case 'X':
            case 'p': {
                unsigned long long v = 0;
                int neg = 0;
                int base = (conv == 'o') ? 8 : 16;

                if (!scan_int(sc, base, width, &neg, &v)) {
                    return assigned;
                }
                if (conv == 'p') {
                    if (!suppress) {
                        *__builtin_va_arg(a, void **) = (void *)(unsigned long)v;
                    }
                } else {
                    store_uint(neg ? (0ULL - v) : v, lenmod, suppress, &a);
                }
                done = 1;
                break;
            }
            case 'e':
            case 'f':
            case 'g':
            case 'E':
            case 'F':
            case 'G': {
                double dv = 0.0;

                if (!scan_double(sc, width, &dv)) {
                    return assigned;
                }
                if (!suppress) {
                    if (lenmod == 1 || lenmod == 2) {
                        *__builtin_va_arg(a, double *) = dv;
                    } else {
                        *__builtin_va_arg(a, float *) = (float)dv;
                    }
                }
                done = 1;
                break;
            }
            case 'c': {
                int n = (width > 0) ? width : 1;
                char *dst = suppress ? (char *)0 : __builtin_va_arg(a, char *);
                int k;

                for (k = 0; k < n; k++) {
                    int c = sc_get(sc);
                    if (c < 0) {
                        return assigned;
                    }
                    if (dst != (char *)0) {
                        dst[k] = (char)c;
                    }
                }
                done = 1;
                break;
            }
            case 's': {
                char *dst = suppress ? (char *)0 : __builtin_va_arg(a, char *);
                int n = 0;
                int c = sc_skip_ws(sc);

                if (c < 0) {
                    return assigned;
                }
                while (c >= 0 && c != ' ' && c != '\t' && c != '\n' &&
                       c != '\r' && c != '\v' && c != '\f' &&
                       (width <= 0 || n < width)) {
                    if (dst != (char *)0) {
                        dst[n] = (char)c;
                    }
                    n++;
                    c = sc_get(sc);
                }
                sc_unget(sc, c);
                if (dst != (char *)0) {
                    dst[n] = '\0';
                }
                done = 1;
                break;
            }
            case 'n': {
                if (!suppress) {
                    store_int(sc->count, lenmod, 0, &a);
                }
                done = 1;
                break;
            }
            default:
                return assigned;
            }

            if (done) {
                if (!suppress) {
                    assigned++;
                }
            }
            fmt++;
        }
    }
    return assigned;
}

int vsscanf(const char *s, const char *fmt, __builtin_va_list ap) {
    src_t sc;

    sc.s = s;
    sc.f = (FILE *)0;
    sc.pos = 0;
    sc.pushed = 0;
    sc.count = 0;
    return scan_core(&sc, fmt, ap);
}

int vfscanf(FILE *f, const char *fmt, __builtin_va_list ap) {
    src_t sc;

    sc.s = (const char *)0;
    sc.f = f;
    sc.pos = 0;
    sc.pushed = 0;
    sc.count = 0;
    return scan_core(&sc, fmt, ap);
}
