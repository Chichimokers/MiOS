#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <mios.h>

/*
 * edit - editor de texto en consola para MiOS (estilo nano, en pequeno).
 *
 *   MiOS> edit hola.c          edita el fichero (lo crea si no existe)
 *   MiOS> edit --selftest      prueba del motor de edicion, sin consola
 *   MiOS> edit --ttytest       prueba del modo crudo de la consola
 *   MiOS> edit --help          esta ayuda
 *
 * La consola se pone en modo crudo mientras se edita (sin eco, sin canonico,
 * sin senales) para poder leer tecla a tecla; al salir se restaura tal como
 * estaba.  La pantalla se pinta por el descriptor 1 con codigos ANSI (el
 * terminal virtual del kernel los entiende); por eso NO se usa cons_* mientras
 * se esta en modo crudo (cons_* manda un '\n' suelto, que en crudo no vuelve
 * al principio de la linea).  cons_* solo se usa antes de entrar y al salir.
 *
 * Teclas:
 *   Flechas, Inicio, Fin, AvPag, RePag   mover el cursor
 *   Enter                                 partir la linea
 *   Retroceso / Supr (ESC[3~)             borrar
 *   Tabulador                             espacios hasta el multiplo de 8
 *   Ctrl-S                                guardar
 *   Ctrl-Q / Ctrl-C                       salir (dos veces si hay cambios)
 *   Ctrl-K                                borrar la linea entera
 *   Ctrl-L                                repintar la pantalla
 *   Ctrl-G                                mostrar/ocultar la ayuda
 *
 * Limitaciones (a proposito): las tabulaciones se expanden a espacios al
 * cargar, al guardar el fichero siempre acaba en '\n' (salvo si esta vacio) y
 * el fichero se carga entero en memoria.
 */

#define TAB 8

/* ------------------------------------------------------------------ */
/* Documento: lista de lineas                                          */
/* ------------------------------------------------------------------ */

typedef struct {
    char *s;    /* contenido, sin '\n', terminado en '\0' */
    int   n;    /* longitud en bytes */
    int   cap;  /* capacidad reservada (incluye el '\0') */
} linea_t;

typedef struct {
    linea_t *v;
    int      n;
    int      cap;
} doc_t;

static int linea_reserva(linea_t *l, int cap) {
    char *p;

    if (l->cap >= cap) {
        return 0;
    }
    if (cap < 16) {
        cap = 16;
    }
    p = realloc(l->s, (size_t)cap);
    if (p == NULL) {
        return -1;
    }
    l->s = p;
    l->cap = cap;
    return 0;
}

/* Inserta el caracter `c` en la columna `col`. */
static int linea_insertar(linea_t *l, int col, int c) {
    if (col < 0) {
        col = 0;
    }
    if (col > l->n) {
        col = l->n;
    }
    if (linea_reserva(l, l->n + 2) != 0) {
        return -1;
    }
    memmove(l->s + col + 1, l->s + col, (size_t)(l->n - col));
    l->s[col] = (char)c;
    l->n++;
    l->s[l->n] = '\0';
    return 0;
}

/* Borra el caracter que esta en la columna `col`. */
static void linea_borrar(linea_t *l, int col) {
    if (col < 0 || col >= l->n) {
        return;
    }
    /* Copia tambien el '\0' final. */
    memmove(l->s + col, l->s + col + 1, (size_t)(l->n - col));
    l->n--;
}

/* Deja la linea con el texto dado, expandiendo tabulaciones a espacios. */
static int linea_texto(linea_t *l, const char *s, int n) {
    int i, col = 0;

    if (linea_reserva(l, n * TAB + 1) != 0) {
        return -1;
    }
    for (i = 0; i < n; i++) {
        if (s[i] == '\t') {
            int k = TAB - (col % TAB);

            while (k-- > 0) {
                l->s[col++] = ' ';
            }
        } else {
            l->s[col++] = s[i];
        }
    }
    l->n = col;
    l->s[col] = '\0';
    return 0;
}

static int doc_insertar_linea(doc_t *d, int pos) {
    linea_t vacia;

    if (d->n == d->cap) {
        int ncap = d->cap ? d->cap * 2 : 32;
        linea_t *p = realloc(d->v, (size_t)ncap * sizeof(linea_t));

        if (p == NULL) {
            return -1;
        }
        d->v = p;
        d->cap = ncap;
    }
    if (pos < 0) {
        pos = 0;
    }
    if (pos > d->n) {
        pos = d->n;
    }
    memmove(d->v + pos + 1, d->v + pos, (size_t)(d->n - pos) * sizeof(linea_t));

    vacia.s = NULL;
    vacia.n = 0;
    vacia.cap = 0;
    d->v[pos] = vacia;
    d->n++;

    if (linea_reserva(&d->v[pos], 16) != 0) {
        return -1;
    }
    d->v[pos].s[0] = '\0';
    return 0;
}

static void doc_borrar_linea(doc_t *d, int pos) {
    if (pos < 0 || pos >= d->n) {
        return;
    }
    free(d->v[pos].s);
    memmove(d->v + pos, d->v + pos + 1,
            (size_t)(d->n - pos - 1) * sizeof(linea_t));
    d->n--;
}

static void doc_liberar(doc_t *d) {
    int i;

    for (i = 0; i < d->n; i++) {
        free(d->v[i].s);
    }
    free(d->v);
    d->v = NULL;
    d->n = 0;
    d->cap = 0;
}

/*
 * Carga el fichero.  Devuelve 0 si va bien y -1 si no se puede leer; si el
 * fichero no existe no es un error: se empieza con una linea vacia y se marca
 * `nuevo` (asi `edit fichero.c` vale tambien para crear ficheros).
 */
static int doc_cargar(doc_t *d, const char *ruta, int *nuevo) {
    int fd, i, ini;
    long total = 0, cap = 0, n;
    char buf[2048];
    char *todo = NULL;

    *nuevo = 0;
    fd = open(ruta, O_RDONLY);
    if (fd < 0) {
        if (errno == ENOENT) {
            *nuevo = 1;
            return doc_insertar_linea(d, 0);
        }
        return -1;
    }

    for (;;) {
        n = (long)read(fd, buf, sizeof(buf));
        if (n < 0) {
            close(fd);
            free(todo);
            return -1;
        }
        if (n == 0) {
            break;
        }
        if (total + n + 1 > cap) {
            char *p;

            cap = (total + n + 1) * 2;
            p = realloc(todo, (size_t)cap);
            if (p == NULL) {
                close(fd);
                free(todo);
                return -1;
            }
            todo = p;
        }
        memcpy(todo + total, buf, (size_t)n);
        total += n;
    }
    close(fd);

    /* Se parte por '\n'; los '\r' de los finales CRLF se descartan. */
    ini = 0;
    for (i = 0; i < total; i++) {
        if (todo[i] == '\n') {
            int fin = i;

            if (fin > ini && todo[fin - 1] == '\r') {
                fin--;
            }
            if (doc_insertar_linea(d, d->n) != 0 ||
                linea_texto(&d->v[d->n - 1], todo + ini, fin - ini) != 0) {
                free(todo);
                return -1;
            }
            ini = i + 1;
        }
    }
    if (ini < total) {              /* ultima linea sin '\n' final */
        int fin = (int)total;

        if (fin > ini && todo[fin - 1] == '\r') {
            fin--;
        }
        if (doc_insertar_linea(d, d->n) != 0 ||
            linea_texto(&d->v[d->n - 1], todo + ini, fin - ini) != 0) {
            free(todo);
            return -1;
        }
    }
    free(todo);

    if (d->n == 0) {                /* fichero vacio */
        return doc_insertar_linea(d, 0);
    }
    return 0;
}

/* Guarda el documento.  Deja en `bytes` lo que se ha escrito. */
static int doc_guardar(doc_t *d, const char *ruta, long *bytes) {
    int fd, i;
    long total = 0;
    int unica_vacia = (d->n == 1 && d->v[0].n == 0);

    fd = open(ruta, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        return -1;
    }

    for (i = 0; i < d->n; i++) {
        if (write(fd, d->v[i].s, (size_t)d->v[i].n) < 0) {
            close(fd);
            return -1;
        }
        total += d->v[i].n;

        /* Salto de linea final, salvo en un documento totalmente vacio. */
        if (!unica_vacia) {
            if (write(fd, "\n", 1) < 0) {
                close(fd);
                return -1;
            }
            total++;
        }
    }
    close(fd);
    *bytes = total;
    return 0;
}

/* ------------------------------------------------------------------ */
/* Operaciones de edicion (sobre el documento y el cursor)              */
/* ------------------------------------------------------------------ */

/* Parte la linea en dos; el cursor pasa al principio de la segunda. */
static void doc_partir(doc_t *d, int *cur, int *col) {
    linea_t *l = &d->v[*cur];
    int cola = l->n - *col;

    if (doc_insertar_linea(d, *cur + 1) != 0) {
        return;
    }
    if (cola > 0) {
        if (linea_reserva(&d->v[*cur + 1], cola + 1) != 0) {
            return;
        }
        memcpy(d->v[*cur + 1].s, l->s + *col, (size_t)cola);
        d->v[*cur + 1].n = cola;
        d->v[*cur + 1].s[cola] = '\0';
        l->n = *col;
        l->s[l->n] = '\0';
    }
    (*cur)++;
    *col = 0;
}

/* Retroceso.  Devuelve 1 si ha cambiado algo. */
static int doc_borrar_antes(doc_t *d, int *cur, int *col) {
    if (*col > 0) {
        linea_borrar(&d->v[*cur], *col - 1);
        (*col)--;
        return 1;
    }
    if (*cur > 0) {
        /* Pegar con la linea de arriba. */
        int arriba = *cur - 1;
        linea_t *a = &d->v[arriba];
        linea_t *l = &d->v[*cur];
        int col_final = a->n;

        if (linea_reserva(a, a->n + l->n + 1) != 0) {
            return 0;
        }
        memcpy(a->s + a->n, l->s, (size_t)l->n);
        a->n += l->n;
        a->s[a->n] = '\0';
        doc_borrar_linea(d, *cur);
        *cur = arriba;
        *col = col_final;
        return 1;
    }
    return 0;
}

/* Suprimir.  Devuelve 1 si ha cambiado algo. */
static int doc_borrar_despues(doc_t *d, int *cur, int *col) {
    linea_t *l = &d->v[*cur];

    if (*col < l->n) {
        linea_borrar(l, *col);
        return 1;
    }
    if (*cur + 1 < d->n) {
        linea_t *sig = &d->v[*cur + 1];

        if (linea_reserva(l, l->n + sig->n + 1) != 0) {
            return 0;
        }
        memcpy(l->s + l->n, sig->s, (size_t)sig->n);
        l->n += sig->n;
        l->s[l->n] = '\0';
        doc_borrar_linea(d, *cur + 1);
        return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Consola: modo crudo y teclas                                        */
/* ------------------------------------------------------------------ */

#define K_FIN_ENTRADA (-1)
#define K_NINGUNA      0
#define K_IZQ          0x100
#define K_DER          0x101
#define K_ARRIBA       0x102
#define K_ABAJO        0x103
#define K_INICIO       0x104
#define K_FIN          0x105
#define K_AVPAG        0x106
#define K_REPAG        0x107
#define K_SUPR         0x108
#define K_DESCONOCIDA  0x109

static struct termios cfg_original;
static struct termios cfg_crudo;
static int crudo_activo = 0;

static int cons_crudo(int fd) {
    struct termios t;

    if (tcgetattr(fd, &cfg_original) != 0) {
        return -1;
    }
    t = cfg_original;
    cfmakeraw(&t);
    t.c_cc[VMIN] = 1;
    t.c_cc[VTIME] = 0;

    if (tcsetattr(fd, TCSANOW, &t) != 0) {
        return -1;
    }
    cfg_crudo = t;
    crudo_activo = 1;
    return 0;
}

static void cons_normal(int fd) {
    if (crudo_activo) {
        tcsetattr(fd, TCSANOW, &cfg_original);
        crudo_activo = 0;
    }
}

/* Lo que mandan las flechas y compania. */
static const struct {
    const char *sec;
    int         tecla;
} SECUENCIAS[] = {
    { "\x1b[A", K_ARRIBA },  { "\x1bOA", K_ARRIBA },
    { "\x1b[B", K_ABAJO },   { "\x1bOB", K_ABAJO },
    { "\x1b[C", K_DER },     { "\x1bOC", K_DER },
    { "\x1b[D", K_IZQ },     { "\x1bOD", K_IZQ },
    { "\x1b[H", K_INICIO },  { "\x1b[1~", K_INICIO }, { "\x1b[7~", K_INICIO },
    { "\x1b[F", K_FIN },     { "\x1b[4~", K_FIN },    { "\x1b[8~", K_FIN },
    { "\x1b[3~", K_SUPR },
    { "\x1b[5~", K_REPAG },
    { "\x1b[6~", K_AVPAG },
    { "\x1b[2~", K_DESCONOCIDA },   /* Insert: se ignora */
};

#define NSEC ((int)(sizeof(SECUENCIAS) / sizeof(SECUENCIAS[0])))

/* (lo leido hasta ahora puede ser el principio de alguna secuencia? */
static int es_prefijo(const char *s, int n) {
    int i;

    for (i = 0; i < NSEC; i++) {
        if (strncmp(SECUENCIAS[i].sec, s, (size_t)n) == 0) {
            return 1;
        }
    }
    return 0;
}

/* -1 si no es una secuencia conocida; si lo es, la tecla K_*. */
static int traducir(const char *s, int n) {
    int i;

    for (i = 0; i < NSEC; i++) {
        if ((int)strlen(SECUENCIAS[i].sec) == n &&
            strncmp(SECUENCIAS[i].sec, s, (size_t)n) == 0) {
            return SECUENCIAS[i].tecla;
        }
    }
    return -1;
}

/* Lee una tecla (las flechas se traducen a K_*). */
static int leer_tecla(int fd) {
    char c;
    char seq[8];
    int n = 0;

    if (read(fd, &c, 1) != 1) {
        return K_FIN_ENTRADA;
    }
    if (c != 27) {
        return (unsigned char)c;
    }

    /*
     * Es un ESC: puede ser una flecha.  Se pasa a "no esperar" (VMIN=0,
     * VTIME=2 = 200 ms) para no quedarse colgado si era un ESC suelto, y se
     * lee hasta completar una secuencia conocida.
     */
    {
        struct termios t = cfg_crudo;

        t.c_cc[VMIN] = 0;
        t.c_cc[VTIME] = 2;
        tcsetattr(fd, TCSANOW, &t);

        while (n < (int)sizeof(seq)) {
            if (read(fd, &c, 1) != 1) {
                break;
            }
            seq[n++] = c;
            if (!es_prefijo(seq, n)) {
                break;
            }
            if (traducir(seq, n) >= 0) {
                break;
            }
        }
        tcsetattr(fd, TCSANOW, &cfg_crudo);

        if (n == 0) {
            return K_NINGUNA;       /* ESC suelto: se ignora */
        }
        n = traducir(seq, n);
        return (n < 0) ? K_DESCONOCIDA : n;
    }
}

/* ------------------------------------------------------------------ */
/* Estado del editor y pintado                                         */
/* ------------------------------------------------------------------ */

typedef struct {
    doc_t doc;
    char  ruta[512];
    int   cur;                  /* linea del cursor (0-based) */
    int   col;                  /* columna del cursor (0-based) */
    int   top;                  /* primera linea visible */
    int   izq;                  /* desplazamiento horizontal */
    int   modificado;
    int   nuevo;
    int   ayuda;
    int   guardado;
    long  bytes;
    char  aviso[160];
    int   filas;
    int   cols;
} editor_t;

#define AYUDA "Ctrl-S guardar   Ctrl-Q salir   Ctrl-K borrar linea   Ctrl-G ayuda"

static char *pant_buf = NULL;

static void buf_add(char *b, int *p, const char *s) {
    while (*s != '\0') {
        b[(*p)++] = *s++;
    }
}

static void aviso(editor_t *e, const char *texto) {
    strncpy(e->aviso, texto, sizeof(e->aviso) - 1);
    e->aviso[sizeof(e->aviso) - 1] = '\0';
}

static void pantalla(editor_t *e, int primera) {
    char *b = pant_buf;
    char num[64];
    int p = 0, r, area = e->filas - 2;

    if (area < 1) {
        area = 1;
    }
    if (e->cur < e->top) {
        e->top = e->cur;
    }
    if (e->cur >= e->top + area) {
        e->top = e->cur - area + 1;
    }
    if (e->top < 0) {
        e->top = 0;
    }
    if (e->col < e->izq) {
        e->izq = e->col;
    }
    if (e->col >= e->izq + e->cols) {
        e->izq = e->col - e->cols + 1;
    }
    if (e->izq < 0) {
        e->izq = 0;
    }

    buf_add(b, &p, primera ? "\x1b[2J\x1b[H" : "\x1b[H");

    for (r = 0; r < area; r++) {
        int li = e->top + r;

        if (li < e->doc.n) {
            linea_t *l = &e->doc.v[li];
            int i;

            for (i = e->izq; i < l->n && i - e->izq < e->cols; i++) {
                b[p++] = l->s[i];
            }
        }
        buf_add(b, &p, "\x1b[K\r\n");
    }

    /* Linea de estado (video inverso). */
    snprintf(num, sizeof(num), " linea %d/%d  col %d ", e->cur + 1,
             e->doc.n, e->col + 1);
    buf_add(b, &p, "\x1b[7m edit  ");
    buf_add(b, &p, e->ruta);
    if (e->nuevo) {
        buf_add(b, &p, " (nuevo)");
    }
    buf_add(b, &p, " ");
    buf_add(b, &p, num);
    buf_add(b, &p, e->modificado ? "[modificado] " : "[guardado] ");
    buf_add(b, &p, "\x1b[K\x1b[0m\r\n");

    /* Ultima linea: aviso, ayuda o el recordatorio minimo. */
    if (e->aviso[0] != '\0') {
        buf_add(b, &p, e->aviso);
    } else if (e->ayuda) {
        buf_add(b, &p, AYUDA);
    } else {
        buf_add(b, &p, "Ctrl-G ayuda");
    }
    buf_add(b, &p, "\x1b[K\r\n");

    /* Cursor a su sitio (1-based). */
    {
        int f = e->cur - e->top + 1;
        int c = e->col - e->izq + 1;

        if (f < 1) {
            f = 1;
        }
        if (f > e->filas - 1) {
            f = e->filas - 1;
        }
        if (c < 1) {
            c = 1;
        }
        if (c > e->cols) {
            c = e->cols;
        }
        snprintf(num, sizeof(num), "\x1b[%d;%dH", f, c);
        buf_add(b, &p, num);
    }

    write(1, b, (size_t)p);
}

/* ------------------------------------------------------------------ */
/* Acciones del editor interactivo                                     */
/* ------------------------------------------------------------------ */

static void ajustar_col(editor_t *e) {
    if (e->col > e->doc.v[e->cur].n) {
        e->col = e->doc.v[e->cur].n;
    }
}

static void mover_pagina(editor_t *e, int dir) {
    int salto = e->filas - 2;

    if (salto < 1) {
        salto = 1;
    }
    e->cur += dir * salto;
    if (e->cur < 0) {
        e->cur = 0;
    }
    if (e->cur >= e->doc.n) {
        e->cur = e->doc.n - 1;
    }
    ajustar_col(e);
}

static void guardar(editor_t *e) {
    long bytes = 0;

    if (doc_guardar(&e->doc, e->ruta, &bytes) != 0) {
        aviso(e, "ERROR: no se ha podido escribir el fichero");
        return;
    }
    e->modificado = 0;
    e->nuevo = 0;
    e->guardado = 1;
    e->bytes = bytes;
    snprintf(e->aviso, sizeof(e->aviso), "guardado: %s (%ld bytes)", e->ruta,
             bytes);
}

static int editar(const char *ruta) {
    editor_t e;
    struct winsize ws;
    int rc = 0, saliendo = 0, confirmando = 0;

    memset(&e, 0, sizeof(e));
    strncpy(e.ruta, ruta, sizeof(e.ruta) - 1);

    /* El fichero se lee antes de tocar la consola: si falla, se avisa normal. */
    if (doc_cargar(&e.doc, ruta, &e.nuevo) != 0) {
        cons_printf("edit: no puedo leer '%s': %s\n", ruta, strerror(errno));
        doc_liberar(&e.doc);
        return 1;
    }

    if (ioctl(0, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 3 && ws.ws_col > 20) {
        e.filas = ws.ws_row;
        e.cols = ws.ws_col;
    } else {
        e.filas = 25;
        e.cols = 80;
    }

    pant_buf = malloc((size_t)(e.filas * (e.cols + 24) + 1024));
    if (pant_buf == NULL) {
        cons_puts("edit: sin memoria para la pantalla\n");
        doc_liberar(&e.doc);
        return 1;
    }

    if (cons_crudo(0) != 0) {
        cons_puts("edit: esto necesita una consola interactiva (no hay tty)\n");
        free(pant_buf);
        pant_buf = NULL;
        doc_liberar(&e.doc);
        return 1;
    }

    e.ayuda = 1;
    if (e.nuevo) {
        aviso(&e, "fichero nuevo: escribe y Ctrl-S para guardar, Ctrl-Q para salir");
    }
    pantalla(&e, 1);

    while (!saliendo) {
        int tecla = leer_tecla(0);

        if (tecla == K_FIN_ENTRADA) {
            rc = 1;
            break;
        }
        if (tecla == K_NINGUNA) {
            continue;               /* ESC suelto: no se hace nada */
        }

        /* Cualquier tecla borra el aviso anterior. */
        e.aviso[0] = '\0';
        if (tecla != 17 && tecla != 3) {
            confirmando = 0;
        }

        if (tecla >= 32 && tecla < 127) {
            if (linea_insertar(&e.doc.v[e.cur], e.col, tecla) == 0) {
                e.col++;
                e.modificado = 1;
            }
        } else {
            switch (tecla) {
            case 9: {           /* Tabulador: espacios hasta el multiplo de 8 */
                int k = TAB - (e.col % TAB);

                while (k-- > 0) {
                    if (linea_insertar(&e.doc.v[e.cur], e.col, ' ') != 0) {
                        break;
                    }
                    e.col++;
                    e.modificado = 1;
                }
                break;
            }
            case 13:
            case 10:
                doc_partir(&e.doc, &e.cur, &e.col);
                e.modificado = 1;
                break;
            case 8:
            case 127:
                if (doc_borrar_antes(&e.doc, &e.cur, &e.col)) {
                    e.modificado = 1;
                }
                break;
            case K_IZQ:
                if (e.col > 0) {
                    e.col--;
                }
                break;
            case K_DER:
                if (e.col < e.doc.v[e.cur].n) {
                    e.col++;
                }
                break;
            case K_ARRIBA:
                if (e.cur > 0) {
                    e.cur--;
                    ajustar_col(&e);
                }
                break;
            case K_ABAJO:
                if (e.cur + 1 < e.doc.n) {
                    e.cur++;
                    ajustar_col(&e);
                }
                break;
            case K_INICIO:
                e.col = 0;
                break;
            case K_FIN:
                e.col = e.doc.v[e.cur].n;
                break;
            case K_AVPAG:
                mover_pagina(&e, 1);
                break;
            case K_REPAG:
                mover_pagina(&e, -1);
                break;
            case K_SUPR:
                if (doc_borrar_despues(&e.doc, &e.cur, &e.col)) {
                    e.modificado = 1;
                }
                break;
            case 19:            /* Ctrl-S: guardar */
                guardar(&e);
                break;
            case 17:            /* Ctrl-Q */
            case 3:             /* Ctrl-C */
                if (e.modificado && !confirmando) {
                    aviso(&e, "hay cambios sin guardar: Ctrl-Q otra vez para salir, Ctrl-S para guardar");
                    confirmando = 1;
                } else {
                    saliendo = 1;
                }
                break;
            case 11: {          /* Ctrl-K: borrar la linea entera */
                if (e.doc.n > 1) {
                    doc_borrar_linea(&e.doc, e.cur);
                    if (e.cur >= e.doc.n) {
                        e.cur = e.doc.n - 1;
                    }
                } else {
                    e.doc.v[0].n = 0;
                    e.doc.v[0].s[0] = '\0';
                }
                e.col = 0;
                e.modificado = 1;
                break;
            }
            case 12:            /* Ctrl-L: repintar */
                pantalla(&e, 1);
                continue;
            case 7:             /* Ctrl-G: ayuda */
                e.ayuda = !e.ayuda;
                break;
            default:
                break;
            }
        }

        pantalla(&e, 0);
    }

    write(1, "\x1b[2J\x1b[H", 7);
    cons_normal(0);
    free(pant_buf);
    pant_buf = NULL;

    if (e.modificado) {
        cons_printf("edit: se sale de '%s' con cambios SIN guardar\n", e.ruta);
    } else if (e.guardado) {
        cons_printf("edit: '%s' guardado (%ld bytes)\n", e.ruta, e.bytes);
    } else {
        cons_printf("edit: '%s' sin cambios\n", e.ruta);
    }
    cons_flush();

    doc_liberar(&e.doc);
    return rc;
}

/* ------------------------------------------------------------------ */
/* Autoprueba (sin consola)                                            */
/* ------------------------------------------------------------------ */

static int linea_es(const doc_t *d, int i, const char *s) {
    return i >= 0 && i < d->n && strcmp(d->v[i].s, s) == 0;
}

static void comprueba(int ok, const char *que, int *fallos) {
    if (ok) {
        cons_printf("  OK: %s\n", que);
    } else {
        cons_printf("  FALLO: %s\n", que);
        (*fallos)++;
    }
}

static int escribir_fichero(const char *ruta, const char *texto) {
    int fd = open(ruta, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd < 0) {
        return -1;
    }
    if (write(fd, texto, strlen(texto)) < 0) {
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

static int leer_fichero(const char *ruta, char *buf, int max) {
    int fd = open(ruta, O_RDONLY);
    int n;

    if (fd < 0) {
        return -1;
    }
    n = (int)read(fd, buf, (size_t)max - 1);
    close(fd);
    if (n < 0) {
        return -1;
    }
    buf[n] = '\0';
    return n;
}

static int selftest(void) {
    const char *ruta = "/tmp/edit-selftest.txt";
    const char *ruta2 = "/tmp/edit-selftest-2.txt";
    const char *ruta_nueva = "/tmp/edit-selftest-nuevo.txt";
    doc_t d;
    int fallos = 0, nuevo = 0;
    long bytes = 0;
    char buf[256];
    int n;

    cons_puts("edit --selftest: prueba del motor de edicion\n");

    /* 1. Cargar un fichero de tres lineas. */
    unlink(ruta_nueva);
    if (escribir_fichero(ruta, "uno\ndos\ntres\n") != 0) {
        cons_puts("  FALLO: no se puede crear el fichero de prueba\n");
        return 1;
    }
    memset(&d, 0, sizeof(d));
    if (doc_cargar(&d, ruta, &nuevo) != 0) {
        cons_puts("  FALLO: no se puede cargar el fichero de prueba\n");
        return 1;
    }
    comprueba(d.n == 3, "carga 3 lineas", &fallos);
    comprueba(linea_es(&d, 0, "uno") && linea_es(&d, 1, "dos") &&
                  linea_es(&d, 2, "tres"),
              "el contenido es el esperado", &fallos);
    comprueba(nuevo == 0, "un fichero que existe no se marca como nuevo", &fallos);

    /* 2. Insertar al principio de la primera linea. */
    comprueba(linea_insertar(&d.v[0], 0, 'X') == 0 && linea_es(&d, 0, "Xuno"),
              "insertar al principio", &fallos);

    /* 3. Partir la segunda linea por la mitad: "dos" -> "do" + "s". */
    {
        int cur = 1, col = 2;

        doc_partir(&d, &cur, &col);
        comprueba(d.n == 4 && linea_es(&d, 1, "do") && linea_es(&d, 2, "s") &&
                      cur == 2 && col == 0,
                  "partir una linea", &fallos);

        /* 4. Y pegarla otra vez (Supr al final de la linea). */
        cur = 1; col = 2;
        comprueba(doc_borrar_despues(&d, &cur, &col) == 1 && d.n == 3 &&
                      linea_es(&d, 1, "dos") && linea_es(&d, 2, "tres"),
                  "pegar dos lineas", &fallos);
    }

    /* 5. Retroceso: borra el caracter insertado. */
    {
        int cur = 0, col = 1;

        comprueba(doc_borrar_antes(&d, &cur, &col) == 1 && linea_es(&d, 0, "uno"),
                  "borrar hacia atras", &fallos);
    }

    /* 6. Guardar y volver a leer. */
    comprueba(doc_guardar(&d, ruta2, &bytes) == 0 && bytes == 13,
              "guardar deja 13 bytes", &fallos);
    n = leer_fichero(ruta2, buf, sizeof(buf));
    comprueba(n == 13 && strcmp(buf, "uno\ndos\ntres\n") == 0,
              "el fichero guardado es identico", &fallos);
    doc_liberar(&d);

    /* 7. Fichero que no existe: se empieza nuevo con una linea vacia. */
    memset(&d, 0, sizeof(d));
    if (doc_cargar(&d, ruta_nueva, &nuevo) != 0) {
        comprueba(0, "abrir un fichero nuevo no falla", &fallos);
    } else {
        comprueba(nuevo == 1 && d.n == 1 && linea_es(&d, 0, ""),
                  "un fichero nuevo empieza con una linea vacia", &fallos);
    }
    doc_liberar(&d);

    /* 8. Fichero vacio. */
    if (escribir_fichero(ruta, "") == 0) {
        memset(&d, 0, sizeof(d));
        doc_cargar(&d, ruta, &nuevo);
        comprueba(d.n == 1 && linea_es(&d, 0, ""),
                  "un fichero vacio da una linea vacia", &fallos);
        comprueba(doc_guardar(&d, ruta2, &bytes) == 0 && bytes == 0,
                  "guardar un documento vacio no escribe nada", &fallos);
        doc_liberar(&d);
    }

    /* 9. Finales CRLF y ultima linea sin salto. */
    escribir_fichero(ruta, "a\r\nb\r\nc");
    memset(&d, 0, sizeof(d));
    doc_cargar(&d, ruta, &nuevo);
    comprueba(d.n == 3 && linea_es(&d, 0, "a") && linea_es(&d, 2, "c"),
              "CRLF y ultima linea sin salto", &fallos);
    doc_liberar(&d);

    /* 10. Tabulaciones expandidas a multiplos de 8: "x<TAB>y" -> "x" + 7esp + "y" */
    escribir_fichero(ruta, "x\ty\n");
    memset(&d, 0, sizeof(d));
    doc_cargar(&d, ruta, &nuevo);
    if (!linea_es(&d, 0, "")) {
        int i, ok = (d.v[0].n == 9);

        for (i = 1; i < 8; i++) {
            if (d.v[0].s[i] != ' ') {
                ok = 0;
            }
        }
        comprueba(ok && d.v[0].s[0] == 'x' && d.v[0].s[8] == 'y',
                  "el tabulador se expande a 8 columnas", &fallos);
    } else {
        comprueba(0, "el tabulador se expande a 8 columnas", &fallos);
    }
    doc_liberar(&d);

    /* 11. Insertar/partir/pegar una linea larga sin perder texto. */
    memset(&d, 0, sizeof(d));
    doc_insertar_linea(&d, 0);
    {
        const char *t = "abcdefghij";
        int i, cur = 0, col = 0;

        for (i = 0; i < 10; i++) {
            linea_insertar(&d.v[0], i, t[i]);
        }
        comprueba(strcmp(d.v[0].s, "abcdefghij") == 0,
                  "10 inserciones dan la linea esperada", &fallos);

        cur = 0; col = 5;
        doc_partir(&d, &cur, &col);
        comprueba(d.n == 2 && linea_es(&d, 0, "abcde") &&
                      linea_es(&d, 1, "fghij"),
                  "partir por la mitad deja las dos mitades", &fallos);

        cur = 0; col = d.v[0].n;    /* final de la primera linea */
        comprueba(doc_borrar_despues(&d, &cur, &col) == 1 && d.n == 1 &&
                      strcmp(d.v[0].s, "abcdefghij") == 0,
                  "insertar/partir/pegar no pierde texto", &fallos);
    }
    doc_liberar(&d);

    unlink(ruta);
    unlink(ruta2);
    cons_printf("edit --selftest: resultado: %d fallo(s)\n", fallos);
    return fallos;
}

/*
 * Prueba del modo crudo de la consola: no lee teclas, solo comprueba que los
 * ioctl de termios funcionan y que los flags se ponen y se vuelven a quitar.
 * Sirve para verificar la parte del editor que depende del tty de verdad
 * (en un chroot no hay /dev/console y no se puede probar).
 */
static int ttytest(void) {
    struct termios orig, t, despues;
    int fallos = 0;

    cons_puts("edit --ttytest: prueba del modo crudo de la consola\n");

    if (tcgetattr(0, &orig) != 0) {
        cons_printf("  FALLO: tcgetattr(0): %s (no hay consola?)\n",
                    strerror(errno));
        return 1;
    }
    cons_printf("  OK: configuracion de la consola leida (lflag=%u)\n",
                orig.c_lflag);

    t = orig;
    cfmakeraw(&t);
    if (tcsetattr(0, TCSANOW, &t) != 0) {
        cons_printf("  FALLO: tcsetattr en modo crudo: %s\n", strerror(errno));
        return 1;
    }
    if (tcgetattr(0, &despues) != 0) {
        cons_puts("  FALLO: no se puede releer la configuracion\n");
        tcsetattr(0, TCSANOW, &orig);
        return 1;
    }
    comprueba((despues.c_lflag & (ICANON | ECHO | ISIG)) == 0,
              "el modo crudo quita ICANON, ECHO e ISIG", &fallos);
    comprueba((despues.c_iflag & IXON) == 0,
              "el modo crudo quita IXON (Ctrl-S llega como tecla)", &fallos);
    comprueba(despues.c_cc[VMIN] == 1 && despues.c_cc[VTIME] == 0,
              "VMIN=1 y VTIME=0 (una tecla por lectura)", &fallos);

    if (tcsetattr(0, TCSANOW, &orig) != 0) {
        cons_printf("  FALLO: no se puede restaurar la consola: %s\n",
                    strerror(errno));
        return fallos + 1;
    }
    despues.c_lflag = 0;
    tcgetattr(0, &despues);
    comprueba(despues.c_lflag == orig.c_lflag,
              "la consola queda como estaba", &fallos);

    cons_printf("edit --ttytest: resultado: %d fallo(s)\n", fallos);
    return fallos;
}

static void ayuda(void) {
    cons_puts("edit - editor de texto de MiOS\n");
    cons_puts("\n");
    cons_puts("uso: edit <fichero>        edita el fichero (lo crea si no existe)\n");
    cons_puts("     edit --selftest       prueba del motor de edicion (sin consola)\n");
    cons_puts("     edit --ttytest        prueba del modo crudo de la consola\n");
    cons_puts("     edit --help           esta ayuda\n");
    cons_puts("\n");
    cons_puts("Teclas:\n");
    cons_puts("  flechas / Inicio / Fin / AvPag / RePag   mover el cursor\n");
    cons_puts("  Enter                                    partir la linea\n");
    cons_puts("  Retroceso / Supr                         borrar\n");
    cons_puts("  Tabulador                                espacios hasta el multiplo de 8\n");
    cons_puts("  Ctrl-S                                   guardar\n");
    cons_puts("  Ctrl-Q (o Ctrl-C)                        salir\n");
    cons_puts("  Ctrl-K                                   borrar la linea entera\n");
    cons_puts("  Ctrl-L                                   repintar la pantalla\n");
    cons_puts("  Ctrl-G                                   mostrar/ocultar la ayuda\n");
    cons_puts("\n");
    cons_puts("Ojo: mientras se edita, la consola esta en modo crudo; al salir se deja\n");
    cons_puts("como estaba.  No conviene poner 'edit' en /etc/autorun (se queda\n");
    cons_puts("esperando teclas y bloquea el arranque).\n");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        ayuda();
        return 2;
    }
    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        ayuda();
        return 0;
    }
    if (strcmp(argv[1], "--selftest") == 0) {
        return selftest();
    }
    if (strcmp(argv[1], "--ttytest") == 0) {
        return ttytest();
    }
    return editar(argv[1]);
}
