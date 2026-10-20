#include "lib/lib.h"
#include "kernel/syscall.h"
#include "../../include/mios.h"

/*
 * MiOS - stdio con FILE*.
 *
 * Sin malloc: cada FILE tiene su buffer dentro de una tabla estatica, asi que
 * stdio funciona aunque todavia no se haya usado el asignador.
 *
 * stdin/stdout/stderr son objetos estaticos y se exportan como punteros de
 * datos (FILE *stdin, ...), lo que ejercita R_X86_64_COPY del cargador.
 */

#define MIOS_BUFSZ   1024
#define MIOS_MAXFILES 16

#define F_RD 1
#define F_WR 2

struct _mios_FILE {
    int         fd;
    int         flags;      /* F_RD / F_WR */
    int         eof;
    int         error;
    int         ungot;      /* caracter devuelto por ungetc (0 = no hay) */
    int         last;       /* 1 escribiendo, -1 leyendo */
    int         unbuffered;
    int         owned;      /* 1 si fopen lo abrio (se cierra en fclose) */
    mios_size_t bufsize;
    mios_size_t rpos;       /* lectura: siguiente byte del buffer */
    mios_size_t rlen;       /* lectura: bytes validos en el buffer */
    mios_size_t wlen;       /* escritura: bytes usados del buffer */
    char       *buf;
};

typedef struct _mios_FILE FILE;

static char std_buf_in[MIOS_BUFSZ];
static char std_buf_out[MIOS_BUFSZ];
static char std_buf_err[MIOS_BUFSZ];

static FILE std_files[3] = {
    { .fd = 0, .flags = F_RD, .bufsize = MIOS_BUFSZ, .buf = std_buf_in },
    { .fd = 1, .flags = F_WR, .bufsize = MIOS_BUFSZ, .buf = std_buf_out },
    { .fd = 2, .flags = F_WR, .bufsize = MIOS_BUFSZ, .buf = std_buf_err,
      .unbuffered = 1 },
};

static FILE file_pool[MIOS_MAXFILES];
static char file_bufs[MIOS_MAXFILES][MIOS_BUFSZ];

FILE *stdin  = &std_files[0];
FILE *stdout = &std_files[1];
FILE *stderr = &std_files[2];

/* ------------------------------------------------------------------ */
/* Buffer interno                                                      */
/* ------------------------------------------------------------------ */

static int file_flush(FILE *f) {
    mios_size_t done = 0;

    if (f == (FILE *)0 || (f->flags & F_WR) == 0 || f->wlen == 0) {
        return 0;
    }
    while (done < f->wlen) {
        long r = mios_write(f->fd, f->buf + done, f->wlen - done);
        if (r <= 0) {
            f->error = 1;
            f->wlen = 0;
            return -1;
        }
        done += (mios_size_t)r;
    }
    f->wlen = 0;
    return 0;
}

/* Deja el buffer preparado para leer (descarta lo escrito antes). */
static int file_prepare_read(FILE *f) {
    if (f->last != -1) {
        if (file_flush(f) != 0) {
            return -1;
        }
        f->last = -1;
    }
    return 0;
}

static int file_fill(FILE *f) {
    long r;

    if (file_prepare_read(f) != 0) {
        return -1;
    }
    r = mios_read(f->fd, f->buf, f->bufsize);
    if (r < 0) {
        f->error = 1;
        f->rlen = 0;
        f->rpos = 0;
        return -1;
    }
    if (r == 0) {
        f->eof = 1;
        f->rlen = 0;
        f->rpos = 0;
        return -1;
    }
    f->rlen = (mios_size_t)r;
    f->rpos = 0;
    return 0;
}

int fgetc(FILE *f) {
    int c;

    if (f == (FILE *)0) {
        return -1;
    }
    if ((f->flags & F_RD) == 0) {
        f->error = 1;
        return -1;
    }
    if (f->ungot != 0) {
        c = f->ungot - 1;
        f->ungot = 0;
        return c;
    }
    if (f->rpos >= f->rlen) {
        if (file_fill(f) != 0) {
            return -1;
        }
    }
    c = (unsigned char)f->buf[f->rpos];
    f->rpos++;
    return c;
}

int getc(FILE *f) {
    return fgetc(f);
}

int getchar(void) {
    return fgetc(stdin);
}

int ungetc(int c, FILE *f) {
    if (f == (FILE *)0 || c < 0) {
        return -1;
    }
    f->ungot = (c & 0xff) + 1;
    f->eof = 0;
    return c & 0xff;
}

int fputc(int c, FILE *f) {
    if (f == (FILE *)0) {
        return -1;
    }
    if ((f->flags & F_WR) == 0) {
        f->error = 1;
        return -1;
    }
    if (f->last != 1) {
        f->last = 1;
    }
    if (f->unbuffered) {
        char ch = (char)c;
        if (mios_write(f->fd, &ch, 1) != 1) {
            f->error = 1;
            return -1;
        }
        return c & 0xff;
    }

    if (f->wlen >= f->bufsize) {
        if (file_flush(f) != 0) {
            return -1;
        }
    }
    f->buf[f->wlen] = (char)c;
    f->wlen++;

    if (c == '\n') {
        if (file_flush(f) != 0) {
            return -1;
        }
    }
    return c & 0xff;
}

int putc(int c, FILE *f) {
    return fputc(c, f);
}

int putchar(int c) {
    return fputc(c, stdout);
}

int fputs(const char *s, FILE *f) {
    while (*s != '\0') {
        if (fputc(*s, f) < 0) {
            return -1;
        }
        s++;
    }
    return 0;
}

int puts(const char *s) {
    if (fputs(s, stdout) < 0) {
        return -1;
    }
    return fputc('\n', stdout);
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *f) {
    size_t total = size * nmemb;
    size_t got = 0;
    unsigned char *p = (unsigned char *)ptr;

    if (size == 0 || nmemb == 0) {
        return 0;
    }
    while (got < total) {
        int c = fgetc(f);
        if (c < 0) {
            break;
        }
        p[got] = (unsigned char)c;
        got++;
    }
    return got / size;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *f) {
    size_t total = size * nmemb;
    size_t done = 0;
    const unsigned char *p = (const unsigned char *)ptr;

    if (size == 0 || nmemb == 0) {
        return 0;
    }
    while (done < total) {
        if (fputc(p[done], f) < 0) {
            break;
        }
        done++;
    }
    return done / size;
}

char *fgets(char *s, int size, FILE *f) {
    int n = 0;

    if (size <= 0) {
        return (char *)0;
    }
    while (n < size - 1) {
        int c = fgetc(f);
        if (c < 0) {
            break;
        }
        s[n] = (char)c;
        n++;
        if (c == '\n') {
            break;
        }
    }
    if (n == 0) {
        return (char *)0;
    }
    s[n] = '\0';
    return s;
}

int fflush(FILE *f) {
    if (f == (FILE *)0) {
        int i;
        int rc = 0;
        for (i = 0; i < MIOS_MAXFILES; i++) {
            if (file_pool[i].buf != (char *)0 && file_pool[i].fd >= 0) {
                if (file_flush(&file_pool[i]) != 0) {
                    rc = -1;
                }
            }
        }
        if (file_flush(stdout) != 0 || file_flush(stderr) != 0) {
            rc = -1;
        }
        return rc;
    }
    return file_flush(f);
}

int feof(FILE *f) {
    return (f != (FILE *)0) ? f->eof : 0;
}

int ferror(FILE *f) {
    return (f != (FILE *)0) ? f->error : 0;
}

void clearerr(FILE *f) {
    if (f != (FILE *)0) {
        f->eof = 0;
        f->error = 0;
    }
}

int fileno(FILE *f) {
    return (f != (FILE *)0) ? f->fd : -1;
}

long ftell(FILE *f) {
    long pos;

    if (f == (FILE *)0) {
        return -1;
    }
    pos = mios_lseek(f->fd, 0, MIOS_SEEK_CUR);
    if (pos < 0) {
        return -1;
    }
    /* Descontar lo que quede en el buffer. */
    if (f->last == -1) {
        pos -= (long)(f->rlen - f->rpos);
    } else {
        pos += (long)f->wlen;
    }
    return pos;
}

int fseek(FILE *f, long offset, int whence) {
    if (f == (FILE *)0) {
        return -1;
    }
    if (file_flush(f) != 0) {
        return -1;
    }
    f->rpos = 0;
    f->rlen = 0;
    f->ungot = 0;
    f->eof = 0;
    f->last = 0;
    if (mios_lseek(f->fd, offset, whence) < 0) {
        return -1;
    }
    return 0;
}

void rewind(FILE *f) {
    if (f != (FILE *)0) {
        fseek(f, 0, MIOS_SEEK_SET);
        f->error = 0;
    }
}

int setvbuf(FILE *f, char *buf, int mode, size_t size) {
    (void)buf;
    (void)size;
    if (f != (FILE *)0 && mode == 2 /* _IONBF */) {
        f->unbuffered = 1;
    }
    return 0;
}

void setbuf(FILE *f, char *buf) {
    (void)f;
    (void)buf;
}

/* ------------------------------------------------------------------ */
/* Apertura y cierre                                                   */
/* ------------------------------------------------------------------ */

static FILE *file_alloc(void) {
    int i;
    for (i = 0; i < MIOS_MAXFILES; i++) {
        if (file_pool[i].buf == (char *)0) {
            FILE *f = &file_pool[i];
            f->fd = -1;
            f->flags = 0;
            f->eof = 0;
            f->error = 0;
            f->ungot = 0;
            f->last = 0;
            f->unbuffered = 0;
            f->owned = 0;
            f->bufsize = MIOS_BUFSZ;
            f->rpos = 0;
            f->rlen = 0;
            f->wlen = 0;
            f->buf = file_bufs[i];
            return f;
        }
    }
    return (FILE *)0;
}

FILE *fdopen(int fd, const char *mode) {
    FILE *f = file_alloc();

    if (f == (FILE *)0) {
        return (FILE *)0;
    }
    f->fd = fd;
    f->flags = (mode[0] == 'r') ? F_RD : F_WR;
    if (mode[0] == 'r' && mode[1] == '+') {
        f->flags = F_RD | F_WR;
    }
    return f;
}

FILE *fopen(const char *path, const char *mode) {
    int flags = 0;
    int fd;
    FILE *f;

    switch (mode[0]) {
    case 'r':
        flags = (mode[1] == '+') ? MIOS_O_RDWR : MIOS_O_RDONLY;
        break;
    case 'w':
        flags = ((mode[1] == '+') ? MIOS_O_RDWR : MIOS_O_WRONLY) | MIOS_O_CREAT |
                MIOS_O_TRUNC;
        break;
    case 'a':
        flags = ((mode[1] == '+') ? MIOS_O_RDWR : MIOS_O_WRONLY) | MIOS_O_CREAT |
                MIOS_O_APPEND;
        break;
    default:
        return (FILE *)0;
    }

    fd = mios_open(path, flags, 0644);
    if (fd < 0) {
        return (FILE *)0;
    }

    f = file_alloc();
    if (f == (FILE *)0) {
        mios_close(fd);
        return (FILE *)0;
    }
    f->fd = fd;
    f->flags = (mode[0] == 'r' && mode[1] != '+') ? F_RD : F_WR;
    if (mode[1] == '+') {
        f->flags = F_RD | F_WR;
    }
    f->owned = 1;
    return f;
}

int fclose(FILE *f) {
    int rc = 0;

    if (f == (FILE *)0) {
        return -1;
    }
    if (file_flush(f) != 0) {
        rc = -1;
    }
    if (f->owned) {
        if (mios_close(f->fd) < 0) {
            rc = -1;
        }
        f->owned = 0;
    }
    if (f != stdin && f != stdout && f != stderr) {
        f->buf = (char *)0; /* devuelve el hueco a la tabla */
        f->fd = -1;
    }
    return rc;
}

FILE *tmpfile(void) {
    static int counter = 0;
    char name[64];
    int fd;
    FILE *f;

    counter++;
    mios_memcpy(name, "/tmp/mios-", 10);
    {
        char num[16];
        int n = 0;
        int i = counter;
        char rev[16];
        int rl = 0;
        do {
            rev[rl] = (char)('0' + (i % 10));
            rl++;
            i /= 10;
        } while (i > 0);
        while (rl > 0) {
            num[n] = rev[--rl];
            n++;
        }
        num[n] = '\0';
        mios_memcpy(name + 10, num, (mios_size_t)n + 1);
    }

    fd = mios_open(name, MIOS_O_RDWR | MIOS_O_CREAT | MIOS_O_EXCL | MIOS_O_TRUNC,
                   0600);
    if (fd < 0) {
        return (FILE *)0;
    }
    mios_unlink(name); /* anonimo: desaparece al cerrar */

    f = file_alloc();
    if (f == (FILE *)0) {
        mios_close(fd);
        return (FILE *)0;
    }
    f->fd = fd;
    f->flags = F_RD | F_WR;
    f->owned = 1;
    return f;
}

int remove(const char *path) {
    return mios_unlink(path);
}

/* ------------------------------------------------------------------ */
/* fprintf / printf / snprintf                                         */
/* ------------------------------------------------------------------ */

typedef struct {
    FILE *f;
    int   count;
} file_ctx_t;

static void file_put(void *ctx, char c) {
    file_ctx_t *w = (file_ctx_t *)ctx;

    fputc(c, w->f);
    w->count++;
}

int vfprintf(FILE *f, const char *fmt, __builtin_va_list ap) {
    file_ctx_t w;

    w.f = f;
    w.count = 0;
    mios_vformat(file_put, &w, fmt, ap);
    return w.count;
}

int fprintf(FILE *f, const char *fmt, ...) {
    __builtin_va_list ap;
    int n;

    __builtin_va_start(ap, fmt);
    n = vfprintf(f, fmt, ap);
    __builtin_va_end(ap);
    return n;
}

int printf(const char *fmt, ...) {
    __builtin_va_list ap;
    int n;

    __builtin_va_start(ap, fmt);
    n = vfprintf(stdout, fmt, ap);
    __builtin_va_end(ap);
    return n;
}

int vprintf(const char *fmt, __builtin_va_list ap) {
    return vfprintf(stdout, fmt, ap);
}

int vfscanf(FILE *f, const char *fmt, __builtin_va_list ap);
int vsscanf(const char *s, const char *fmt, __builtin_va_list ap);

int fscanf(FILE *f, const char *fmt, ...) {
    __builtin_va_list ap;
    int n;

    __builtin_va_start(ap, fmt);
    n = vfscanf(f, fmt, ap);
    __builtin_va_end(ap);
    return n;
}

int scanf(const char *fmt, ...) {
    __builtin_va_list ap;
    int n;

    __builtin_va_start(ap, fmt);
    n = vfscanf(stdin, fmt, ap);
    __builtin_va_end(ap);
    return n;
}

/* ------------------------------------------------------------------ */
/* snprintf / sprintf                                                  */
/* ------------------------------------------------------------------ */

int vsnprintf(char *buf, size_t size, const char *fmt, __builtin_va_list ap) {
    return mios_vsnprintf(buf, (mios_size_t)size, fmt, ap);
}

int snprintf(char *buf, size_t size, const char *fmt, ...) {
    __builtin_va_list ap;
    int n;

    __builtin_va_start(ap, fmt);
    n = mios_vsnprintf(buf, (mios_size_t)size, fmt, ap);
    __builtin_va_end(ap);
    return n;
}

int vsprintf(char *buf, const char *fmt, __builtin_va_list ap) {
    return mios_vsnprintf(buf, (mios_size_t)-1, fmt, ap);
}

int sprintf(char *buf, const char *fmt, ...) {
    __builtin_va_list ap;
    int n;

    __builtin_va_start(ap, fmt);
    n = mios_vsnprintf(buf, (mios_size_t)-1, fmt, ap);
    __builtin_va_end(ap);
    return n;
}

int sscanf(const char *s, const char *fmt, ...) {
    __builtin_va_list ap;
    int n;

    __builtin_va_start(ap, fmt);
    n = vsscanf(s, fmt, ap);
    __builtin_va_end(ap);
    return n;
}

void perror(const char *s) {
    int err = *__errno_location();

    if (s != (const char *)0 && *s != '\0') {
        fprintf(stderr, "%s: %s\n", s, mios_strerror(err));
    } else {
        fprintf(stderr, "%s\n", mios_strerror(err));
    }
}
