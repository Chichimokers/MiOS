#include "lib/lib.h"
#include "kernel/syscall.h"
#include "../../include/mios.h"

/*
 * MiOS - stdlib.h: conversion, ordenacion, entorno y salida del programa.
 *
 * Nada de esto usa libc: los numeros se analizan y se formatean a mano.
 */

/* ------------------------------------------------------------------ */
/* Conversion                                                          */
/* ------------------------------------------------------------------ */

/* Nucleo de strtoull: devuelve el valor y, por punteros, si hubo digitos,
 * el fin del numero y un desbordamiento. */
static unsigned long long scan_ull(const char *s, char **endp, int base,
                                   int *any, int *ovf) {
    const char *p = s;
    unsigned long long v = 0;
    int d, over = 0;

    *any = 0;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == '\v' ||
           *p == '\f') {
        p++;
    }
    if (*p == '+' || *p == '-') {
        p++;
    }
    if ((base == 0 || base == 16) && p[0] == '0' && (p[1] == 'x' || p[1] == 'X') &&
        ((p[2] >= '0' && p[2] <= '9') || (p[2] >= 'a' && p[2] <= 'f') ||
         (p[2] >= 'A' && p[2] <= 'F'))) {
        base = 16;
        p += 2;
    } else if (base == 0 && p[0] == '0') {
        base = 8;
    } else if (base == 0) {
        base = 10;
    }

    for (;;) {
        char c = *p;
        if (c >= '0' && c <= '9') {
            d = c - '0';
        } else if (c >= 'a' && c <= 'z') {
            d = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'Z') {
            d = c - 'A' + 10;
        } else {
            break;
        }
        if (d >= base) {
            break;
        }
        if (v > (18446744073709551615ULL - (unsigned long long)d) / (unsigned long long)base) {
            over = 1;
        }
        v = v * (unsigned long long)base + (unsigned long long)d;
        *any = 1;
        p++;
    }

    *ovf = over;
    if (endp != (char **)0) {
        *endp = (char *)(*any ? p : s);
    }
    return v;
}

unsigned long long strtoull(const char *s, char **endp, int base) {
    int any = 0, ovf = 0;
    unsigned long long v;

    v = scan_ull(s, endp, base, &any, &ovf);
    if (ovf) {
        errno = ERANGE;
        return 18446744073709551615ULL;
    }
    return v;
}

unsigned long strtoul(const char *s, char **endp, int base) {
    return (unsigned long)strtoull(s, endp, base);
}

long long strtoll(const char *s, char **endp, int base) {
    const char *p = s;
    int neg = 0, any = 0, ovf = 0;
    unsigned long long v;

    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == '\v' ||
           *p == '\f') {
        p++;
    }
    if (*p == '-') {
        neg = 1;
    }
    v = scan_ull(s, endp, base, &any, &ovf);
    if (ovf) {
        errno = ERANGE;
        return neg ? (-9223372036854775807LL - 1) : 9223372036854775807LL;
    }
    if (neg) {
        if (v > 9223372036854775808ULL) {
            errno = ERANGE;
            return -9223372036854775807LL - 1;
        }
        return (long long)(0ULL - v);
    }
    if (v > 9223372036854775807ULL) {
        errno = ERANGE;
        return 9223372036854775807LL;
    }
    return (long long)v;
}

long strtol(const char *s, char **endp, int base) {
    return (long)strtoll(s, endp, base);
}

int atoi(const char *s) {
    return (int)strtoll(s, (char **)0, 10);
}

long atol(const char *s) {
    return (long)strtoll(s, (char **)0, 10);
}

long long atoll(const char *s) {
    return strtoll(s, (char **)0, 10);
}

/* Compara sin distinguir mayusculas (prefijo de "inf"/"infinity"/"nan"). */
static int match_ci(const char *s, const char *word) {
    while (*word != '\0') {
        char a = *s, b = *word;
        if (a >= 'A' && a <= 'Z') {
            a = (char)(a - 'A' + 'a');
        }
        if (b >= 'A' && b <= 'Z') {
            b = (char)(b - 'A' + 'a');
        }
        if (a != b) {
            return 0;
        }
        s++;
        word++;
    }
    return 1;
}

double strtod(const char *s, char **endp) {
    const char *p = s;
    const char *start = s;
    int neg = 0;
    int any = 0;
    double v = 0.0;

    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == '\v' ||
           *p == '\f') {
        p++;
    }
    if (*p == '+' || *p == '-') {
        neg = (*p == '-');
        p++;
    }

    if (match_ci(p, "inf")) {
        p += 3;
        if (match_ci(p, "inity")) {
            p += 5;
        }
        if (endp != (char **)0) {
            *endp = (char *)p;
        }
        return neg ? -1.0 / 0.0 : 1.0 / 0.0;
    }
    if (match_ci(p, "nan")) {
        p += 3;
        if (endp != (char **)0) {
            *endp = (char *)p;
        }
        return 0.0 / 0.0;
    }

    while (*p >= '0' && *p <= '9') {
        v = v * 10.0 + (double)(*p - '0');
        p++;
        any = 1;
    }
    if (*p == '.') {
        double scale = 0.1;
        p++;
        while (*p >= '0' && *p <= '9') {
            v += (double)(*p - '0') * scale;
            scale *= 0.1;
            p++;
            any = 1;
        }
    }
    if (any && (*p == 'e' || *p == 'E')) {
        const char *save = p;
        int eneg = 0;
        int e = 0;
        int eany = 0;

        p++;
        if (*p == '+' || *p == '-') {
            eneg = (*p == '-');
            p++;
        }
        while (*p >= '0' && *p <= '9') {
            e = e * 10 + (*p - '0');
            if (e > 1000) {
                e = 1000;
            }
            p++;
            eany = 1;
        }
        if (eany) {
            while (e-- > 0) {
                v = eneg ? v / 10.0 : v * 10.0;
            }
        } else {
            p = save;
        }
    }

    if (endp != (char **)0) {
        *endp = (char *)(any ? p : start);
    }
    return neg ? -v : v;
}

float strtof(const char *s, char **endp) {
    return (float)strtod(s, endp);
}

/* La parte "long double" de MiOS (x87, 80 bits) no tiene un analizador propio:
 * se pasa por double.  Es suficiente (tcc lo usa para leer literales y
 * constantes) y queda anotado como limite conocido de la libc. */
long double strtold(const char *s, char **endp) {
    return (long double)strtod(s, endp);
}

double atof(const char *s) {
    return strtod(s, (char **)0);
}

/* ------------------------------------------------------------------ */
/* Ordenacion y busqueda                                               */
/* ------------------------------------------------------------------ */

static void swap_bytes(char *a, char *b, size_t n) {
    while (n-- > 0) {
        char t = *a;
        *a = *b;
        *b = t;
        a++;
        b++;
    }
}

static void ins_sort(char *a, size_t n, size_t sz,
                     int (*cmp)(const void *, const void *)) {
    size_t i, j;

    for (i = 1; i < n; i++) {
        j = i;
        while (j > 0 && cmp(a + (j - 1) * sz, a + j * sz) > 0) {
            swap_bytes(a + (j - 1) * sz, a + j * sz, sz);
            j--;
        }
    }
}

static void qs(char *a, size_t n, size_t sz,
               int (*cmp)(const void *, const void *)) {
    size_t i, j;
    char *piv;

    if (n < 12) {
        ins_sort(a, n, sz, cmp);
        return;
    }

    /* Pivote: elemento central, apartado al final. */
    swap_bytes(a + (n / 2) * sz, a + (n - 1) * sz, sz);
    piv = a + (n - 1) * sz;

    i = 0;
    for (j = 0; j < n - 1; j++) {
        if (cmp(a + j * sz, piv) <= 0) {
            if (i != j) {
                swap_bytes(a + i * sz, a + j * sz, sz);
            }
            i++;
        }
    }
    swap_bytes(a + i * sz, piv, sz);

    qs(a, i, sz, cmp);
    qs(a + (i + 1) * sz, n - i - 1, sz, cmp);
}

void qsort(void *base, size_t nmemb, size_t size,
           int (*cmp)(const void *, const void *)) {
    if (base == (void *)0 || nmemb < 2 || size == 0) {
        return;
    }
    qs((char *)base, nmemb, size, cmp);
}

void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
              int (*cmp)(const void *, const void *)) {
    size_t lo = 0;
    size_t hi = nmemb;

    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        const char *p = (const char *)base + mid * size;
        int r = cmp(key, p);

        if (r == 0) {
            return (void *)p;
        }
        if (r < 0) {
            hi = mid;
        } else {
            lo = mid + 1;
        }
    }
    return (void *)0;
}

/* ------------------------------------------------------------------ */
/* Aritmetica                                                          */
/* ------------------------------------------------------------------ */

int abs(int v) {
    return (v < 0) ? -v : v;
}

long labs(long v) {
    return (v < 0) ? -v : v;
}

long long llabs(long long v) {
    return (v < 0) ? -v : v;
}

div_t div(int num, int den) {
    div_t r;
    r.quot = num / den;
    r.rem = num % den;
    return r;
}

ldiv_t ldiv(long num, long den) {
    ldiv_t r;
    r.quot = num / den;
    r.rem = num % den;
    return r;
}

lldiv_t lldiv(long long num, long long den) {
    lldiv_t r;
    r.quot = num / den;
    r.rem = num % den;
    return r;
}

/* ------------------------------------------------------------------ */
/* Aleatorios                                                          */
/* ------------------------------------------------------------------ */

static unsigned long rng_state = 1;

void srand(unsigned int seed) {
    rng_state = (unsigned long)(seed ? seed : 1);
}

int rand(void) {
    /* Congruencial lineal (Numerical Recipes). */
    rng_state = (1103515245UL * rng_state + 12345UL) & 0x7fffffffUL;
    return (int)rng_state;
}

/* ------------------------------------------------------------------ */
/* Entorno                                                             */
/* ------------------------------------------------------------------ */

char **environ = 0;

void __mios_set_environ(char **envp) {
    environ = envp;
}

static size_t env_len(void) {
    size_t n = 0;

    if (environ == (char **)0) {
        return 0;
    }
    while (environ[n] != (char *)0) {
        n++;
    }
    return n;
}

char *getenv(const char *name) {
    size_t n = strlen(name);
    char **e;

    if (environ == (char **)0) {
        return (char *)0;
    }
    for (e = environ; *e != (char *)0; e++) {
        if (strncmp(*e, name, n) == 0 && (*e)[n] == '=') {
            return *e + n + 1;
        }
    }
    return (char *)0;
}

static char **env_index(const char *name, size_t n) {
    char **e;

    if (environ == (char **)0) {
        return (char **)0;
    }
    for (e = environ; *e != (char *)0; e++) {
        if (strncmp(*e, name, n) == 0 && (*e)[n] == '=') {
            return e;
        }
    }
    return (char **)0;
}

int setenv(const char *name, const char *value, int overwrite) {
    size_t n = strlen(name);
    size_t vlen = strlen(value);
    char **slot;
    char *entry;

    if (name == (const char *)0 || *name == '\0' || strchr(name, '=') != (char *)0) {
        errno = EINVAL;
        return -1;
    }

    slot = env_index(name, n);
    if (slot != (char **)0) {
        if (!overwrite) {
            return 0;
        }
        entry = (char *)malloc(n + vlen + 2);
        if (entry == (char *)0) {
            return -1;
        }
        memcpy(entry, name, n);
        entry[n] = '=';
        memcpy(entry + n + 1, value, vlen + 1);
        *slot = entry; /* el anterior era del llamante o nuestro: no se libera
                        * para no invalidar punteros que alguien pueda tener */
        return 0;
    }

    {
        size_t count = env_len();
        char **arr = (char **)malloc((count + 2) * sizeof(char *));
        size_t i;

        if (arr == (char **)0) {
            return -1;
        }
        for (i = 0; i < count; i++) {
            arr[i] = environ[i];
        }
        entry = (char *)malloc(n + vlen + 2);
        if (entry == (char *)0) {
            free(arr);
            return -1;
        }
        memcpy(entry, name, n);
        entry[n] = '=';
        memcpy(entry + n + 1, value, vlen + 1);
        arr[count] = entry;
        arr[count + 1] = (char *)0;
        environ = arr;
    }
    return 0;
}

int putenv(char *string) {
    char *eq = strchr(string, '=');
    size_t n;
    char **slot;

    if (eq == (char *)0) {
        return unsetenv(string);
    }
    n = (size_t)(eq - string);
    slot = env_index(string, n);
    if (slot != (char **)0) {
        *slot = string;
        return 0;
    }
    {
        size_t count = env_len();
        char **arr = (char **)malloc((count + 2) * sizeof(char *));
        size_t i;

        if (arr == (char **)0) {
            return -1;
        }
        for (i = 0; i < count; i++) {
            arr[i] = environ[i];
        }
        arr[count] = string;
        arr[count + 1] = (char *)0;
        environ = arr;
    }
    return 0;
}

int unsetenv(const char *name) {
    size_t n = strlen(name);
    char **slot;

    if (name == (const char *)0 || *name == '\0' || strchr(name, '=') != (char *)0) {
        errno = EINVAL;
        return -1;
    }
    slot = env_index(name, n);
    if (slot == (char **)0) {
        return 0;
    }
    while (*slot != (char *)0) {
        slot[0] = slot[1];
        slot++;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Salida del programa                                                 */
/* ------------------------------------------------------------------ */

#define MAX_ATEXIT 32

static void (*exit_fns[MAX_ATEXIT])(void);
static int exit_n = 0;

static struct {
    void (*fn)(void *);
    void *arg;
} cxa_fns[MAX_ATEXIT];
static int cxa_n = 0;

int atexit(void (*fn)(void)) {
    if (exit_n >= MAX_ATEXIT) {
        return -1;
    }
    exit_fns[exit_n] = fn;
    exit_n++;
    return 0;
}

int __cxa_atexit(void (*fn)(void *), void *arg, void *dso) {
    (void)dso;
    if (cxa_n >= MAX_ATEXIT) {
        return -1;
    }
    cxa_fns[cxa_n].fn = fn;
    cxa_fns[cxa_n].arg = arg;
    cxa_n++;
    return 0;
}

void __cxa_finalize(void *dso) {
    (void)dso;
    while (cxa_n > 0) {
        cxa_n--;
        if (cxa_fns[cxa_n].fn != (void (*)(void *))0) {
            cxa_fns[cxa_n].fn(cxa_fns[cxa_n].arg);
        }
    }
}

void exit(int status) {
    int i;

    for (i = exit_n - 1; i >= 0; i--) {
        if (exit_fns[i] != (void (*)(void))0) {
            exit_fns[i]();
        }
    }
    __cxa_finalize((void *)0);
    fflush((FILE *)0);
    cons_flush();
    mios_exit(status);
}

void _Exit(int status) {
    mios_exit(status);
}

void _exit(int status) {
    mios_exit(status);
}

void abort(void) {
    fflush((FILE *)0);
    cons_flush();
    mios_exit(134); /* 128 + SIGABRT */
}

/* ------------------------------------------------------------------ */
/* Ficheros temporales (mkstemp/mkstemps/mkdtemp)                      */
/* ------------------------------------------------------------------ */
/* Fase 7: libiberty (y por tanto binutils) los usa para los ficheros
 * temporales de 'ld' y 'as'.  El nombre se completa con una mezcla de
 * getpid()/gettimeofday() y una secuencia congruente (no hace falta un
 * generador bueno: basta con no repetir y que O_EXCL detecte el choque). */

static const char tmp_letters[] =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

static unsigned int tmp_seed(void) {
    struct mios_timeval tv;
    unsigned int s = (unsigned int)mios_getpid();

    if (mios_gettimeofday(&tv, MIOS_NULL) == 0) {
        s = s * 1103515245u + (unsigned int)tv.tv_sec * 2654435761u +
            (unsigned int)tv.tv_usec;
    }
    return s;
}

/* Comprueba que la plantilla acaba en XXXXXX (antes del sufijo) y devuelve
 * donde empieza. */
static char *tmp_position(char *pattern, int suffix_len) {
    mios_size_t len = (mios_size_t)mios_strlen(pattern);

    if ((int)len < 6 + suffix_len) {
        return MIOS_NULL;
    }
    if (mios_strncmp(pattern + len - 6 - suffix_len, "XXXXXX", 6) != 0) {
        return MIOS_NULL;
    }
    return pattern + len - 6 - suffix_len;
}

int mkstemps(char *pattern, int suffix_len) {
    char *x = tmp_position(pattern, suffix_len);
    unsigned int r = tmp_seed();
    int intento;

    if (x == MIOS_NULL) {
        errno = EINVAL;
        return -1;
    }

    for (intento = 0; intento < TMP_MAX; intento++) {
        int i, fd;

        for (i = 0; i < 6; i++) {
            r = r * 1103515245u + 12345u;
            x[i] = tmp_letters[(r >> 16) % 62];
        }
        fd = open(pattern, O_CREAT | O_EXCL | O_RDWR, 0600);
        if (fd >= 0) {
            return fd;
        }
        if (errno != EEXIST) {
            return -1;
        }
    }
    errno = EEXIST;
    return -1;
}

int mkstemp(char *pattern) {
    return mkstemps(pattern, 0);
}

/* mktemp(): como mkstemp pero SIN crear el fichero (y por tanto sin garantia
 * contra carreras: es la version clasica, la que usa binutils para los nombres
 * temporales que luego abre el mismo).  Devuelve la plantilla completada, o
 * una cadena vacia si no encuentra nombre libre (como glibc). */
char *mktemp(char *pattern) {
    char *x = tmp_position(pattern, 0);
    unsigned int r = tmp_seed();
    int intento;

    if (x == MIOS_NULL) {
        errno = EINVAL;
        pattern[0] = '\0';
        return pattern;
    }

    for (intento = 0; intento < TMP_MAX; intento++) {
        int i;

        for (i = 0; i < 6; i++) {
            r = r * 1103515245u + 12345u;
            x[i] = tmp_letters[(r >> 16) % 62];
        }
        if (mios_access(pattern, 0) != 0) {   /* 0 = F_OK: no existe */
            return pattern;
        }
    }
    errno = EEXIST;
    pattern[0] = '\0';
    return pattern;
}

/* Conversion a los tipos maximos de <inttypes.h> (Fase 7). */
intmax_t strtoimax(const char *nptr, char **endptr, int base) {
    return (intmax_t)strtoll(nptr, endptr, base);
}

uintmax_t strtoumax(const char *nptr, char **endptr, int base) {
    return (uintmax_t)strtoull(nptr, endptr, base);
}

intmax_t imaxabs(intmax_t v) {
    return v < 0 ? -v : v;
}

imaxdiv_t imaxdiv(intmax_t num, intmax_t den) {
    imaxdiv_t r;

    r.quot = num / den;
    r.rem  = num % den;
    return r;
}

char *mkdtemp(char *pattern) {
    char *x = tmp_position(pattern, 0);
    unsigned int r = tmp_seed();
    int intento;

    if (x == MIOS_NULL) {
        errno = EINVAL;
        return MIOS_NULL;
    }

    for (intento = 0; intento < TMP_MAX; intento++) {
        int i;

        for (i = 0; i < 6; i++) {
            r = r * 1103515245u + 12345u;
            x[i] = tmp_letters[(r >> 16) % 62];
        }
        if (mios_mkdir(pattern, 0700) == 0) {
            return pattern;
        }
        if (errno != EEXIST) {
            return MIOS_NULL;
        }
    }
    errno = EEXIST;
    return MIOS_NULL;
}
