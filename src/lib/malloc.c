#include "lib/lib.h"
#include "kernel/syscall.h"
#include "../../include/mios.h"

/*
 * MiOS - memoria dinamica: malloc / calloc / realloc / free.
 *
 * Asignador propio, sencillo pero correcto:
 *   - La memoria se pide al kernel con brk(), asi que todos los bloques son
 *     contiguos y se pueden fusionar vecinos sin ambiguedad.
 *   - Lista de bloques en orden de direccion; cabecera de 32 bytes (alineada),
 *     por lo que el puntero que se devuelve queda alineado a 16.
 *   - Primer ajuste (first fit) + division del sobrante + fusion al liberar.
 *
 * No hay hilos en MiOS, asi que no hace falta bloqueo.
 */

#define ALIGN 16
#define ALIGN_UP(x) (((x) + (ALIGN - 1)) & ~(mios_size_t)(ALIGN - 1))

/* Resto minimo que merece la pena dejar como bloque libre al dividir. */
#define MIN_SPLIT (sizeof(blk_t) + ALIGN)

/* Tamano minimo de cada peticion al kernel. */
#define CHUNK_MIN (64 * 1024)

typedef struct blk {
    mios_size_t   size;  /* bytes totales del bloque, cabecera incluida */
    struct blk   *prev;
    struct blk   *next;
    int           free;
    int           pad;
} blk_t;

_Static_assert(sizeof(blk_t) == 32, "la cabecera del bloque debe medir 32 bytes");

#define HDR ((mios_size_t)sizeof(blk_t))

static blk_t *heap_first = MIOS_NULL;
static blk_t *heap_last  = MIOS_NULL;

static void heap_add(blk_t *b) {
    b->prev = heap_last;
    b->next = MIOS_NULL;
    if (heap_last != MIOS_NULL) {
        heap_last->next = b;
    } else {
        heap_first = b;
    }
    heap_last = b;
}

/* Pide mas memoria al kernel y la anade al final del monton. */
static int heap_grow(mios_size_t need) {
    long cur, want, got;
    mios_size_t total = need + HDR;
    blk_t *b;

    if (total < CHUNK_MIN) {
        total = CHUNK_MIN;
    }
    total = ALIGN_UP(total);

    cur = mios_brk(MIOS_NULL);
    if (cur <= 0 || mios_error(cur)) {
        return -1;
    }
    want = cur + (long)total;
    got = mios_brk((void *)want);
    if (got < want) {
        return -1; /* sin memoria */
    }

    b = (blk_t *)cur;
    b->size = total;
    b->free = 1;
    heap_add(b);
    return 0;
}

/* Parte 'b' para que ocupe exactamente 'need' si sobra espacio suficiente. */
static void split(blk_t *b, mios_size_t need) {
    blk_t *rest;

    if (b->size < need + MIN_SPLIT) {
        return;
    }
    rest = (blk_t *)((char *)b + need);
    rest->size = b->size - need;
    rest->free = 1;
    rest->prev = b;
    rest->next = b->next;
    if (rest->next != MIOS_NULL) {
        rest->next->prev = rest;
    } else {
        heap_last = rest;
    }
    b->next = rest;
    b->size = need;
}

void *malloc(mios_size_t n) {
    blk_t *b;
    mios_size_t need;

    if (n == 0) {
        n = 1;
    }
    need = ALIGN_UP(n) + HDR;
    if (need < n) {
        return MIOS_NULL; /* desbordamiento */
    }

    for (b = heap_first; b != MIOS_NULL; b = b->next) {
        if (!b->free || b->size < need) {
            continue;
        }
        split(b, need);
        b->free = 0;
        return (void *)((char *)b + HDR);
    }

    if (heap_grow(need) != 0) {
        return MIOS_NULL;
    }
    return malloc(n); /* ahora ya cabe */
}

void free(void *p) {
    blk_t *b, *q;

    if (p == MIOS_NULL) {
        return;
    }
    b = (blk_t *)((char *)p - HDR);
    b->free = 1;

    /* Unir con el bloque siguiente si esta libre. */
    q = b->next;
    if (q != MIOS_NULL && q->free) {
        b->size += q->size;
        b->next = q->next;
        if (q->next != MIOS_NULL) {
            q->next->prev = b;
        } else {
            heap_last = b;
        }
    }

    /* Unir con el bloque anterior si esta libre. */
    q = b->prev;
    if (q != MIOS_NULL && q->free) {
        q->size += b->size;
        q->next = b->next;
        if (b->next != MIOS_NULL) {
            b->next->prev = q;
        } else {
            heap_last = q;
        }
    }
}

void *calloc(mios_size_t nmemb, mios_size_t size) {
    mios_size_t total;
    void *p;

    if (nmemb == 0 || size == 0) {
        nmemb = 1;
        size = 1;
    }
    total = nmemb * size;
    if (total / size != nmemb) {
        return MIOS_NULL; /* desbordamiento */
    }

    p = malloc(total);
    if (p != MIOS_NULL) {
        mios_memset(p, 0, total);
    }
    return p;
}

void *realloc(void *p, mios_size_t n) {
    blk_t *b, *q;
    mios_size_t need, copy;
    void *np;

    if (p == MIOS_NULL) {
        return malloc(n);
    }
    if (n == 0) {
        free(p);
        return MIOS_NULL;
    }

    b = (blk_t *)((char *)p - HDR);
    need = ALIGN_UP(n) + HDR;

    if (b->size >= need) {
        split(b, need);
        return p;
    }

    /* Si el bloque siguiente esta libre y entre los dos alcanza, se fusiona. */
    q = b->next;
    if (q != MIOS_NULL && q->free && b->size + q->size >= need) {
        b->size += q->size;
        b->next = q->next;
        if (q->next != MIOS_NULL) {
            q->next->prev = b;
        } else {
            heap_last = b;
        }
        split(b, need);
        return p;
    }

    np = malloc(n);
    if (np == MIOS_NULL) {
        return MIOS_NULL;
    }
    copy = b->size - HDR;
    if (copy > n) {
        copy = n;
    }
    mios_memcpy(np, p, copy);
    free(p);
    return np;
}

/* Cuanto se puede usar realmente de un bloque (util para depurar). */
mios_size_t malloc_usable_size(void *p) {
    blk_t *b;

    if (p == MIOS_NULL) {
        return 0;
    }
    b = (blk_t *)((char *)p - HDR);
    return b->size - HDR;
}
