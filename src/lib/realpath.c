#include "lib/lib.h"
#include "lib/ret.h"
#include "kernel/syscall.h"
#include "../../include/mios.h"
#include "../../include/stdlib.h"
#include "../../include/unistd.h"
#include "../../include/limits.h"
#include "../../include/errno.h"

/*
 * MiOS - rutas canonicas (stdlib.h): realpath() y canonicalize_file_name().
 *
 * realpath() convierte una ruta en absoluta, resuelve los enlaces simbolicos y
 * quita los "." y "..".  Es la clase de funcion que necesita el software que se
 * porta a MiOS: libiberty (y por tanto binutils) solo usa su implementacion
 * buena si el sistema tiene realpath() o canonicalize_file_name(); si no,
 * lrealpath() acaba sin codigo y devuelve basura (asi se cayo nuestro 'ld').
 *
 * Solo se usan llamadas que MiOS ya tiene: lstat (para ver de que tipo es cada
 * componente), readlink (para el destino de los enlaces) y getcwd (para la
 * parte relativa).  Algoritmo: se recorren los componentes de la ruta
 * construyendo la canonica en 'out'; cuando un componente resulta ser un
 * enlace, su destino se pone delante de lo que queda por procesar y, si es
 * absoluto, se empieza de cero.  Como 'out' es siempre el directorio que
 * contiene al enlace, los destinos relativos se resuelven bien.
 */

#define RP_MAXLINKS 40              /* igual que el limite de Linux */

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/* Copia src en dst (dst tiene PATH_MAX bytes); devuelve la longitud o -1. */
static int rp_copy(char *dst, const char *src) {
    mios_size_t n = mios_strlen(src);

    if (n + 1 > (mios_size_t)PATH_MAX) {
        errno = ENAMETOOLONG;
        return -1;
    }
    mios_memcpy(dst, src, n + 1);
    return (int)n;
}

/* out = "..." (len) + "/comp".  Devuelve la nueva longitud o -1. */
static int rp_append(char *out, int len, const char *comp, int clen) {
    if (len + 1 + clen + 1 > PATH_MAX) {
        errno = ENAMETOOLONG;
        return -1;
    }
    out[len] = '/';
    mios_memcpy(out + len + 1, comp, (mios_size_t)clen + 1);
    return len + 1 + clen;
}

/* out = directorio padre de "..." (len): quita el ultimo componente. */
static int rp_up(char *out, int len) {
    while (len > 0 && out[len - 1] != '/') {
        len--;
    }
    if (len > 0) {
        len--;                      /* quitar tambien la barra */
    }
    out[len] = '\0';
    return len;
}

char *realpath(const char *path, char *resolved) {
    char out[PATH_MAX];             /* canonica acumulada */

    char rest[PATH_MAX];            /* componentes por procesar */

    char comp[PATH_MAX];            /* componente actual */

    char link[PATH_MAX];            /* destino de un enlace */

    char cand[PATH_MAX];            /* candidato: out + "/" + comp */

    char nuevo[PATH_MAX];           /* destino + resto (para enlaces) */

    struct mios_stat st;
    int olen = 0;
    int rpos = 0;
    int links = 0;
    char *r;

    if (path == MIOS_NULL) {
        errno = EINVAL;
        return MIOS_NULL;
    }
    if (path[0] == '\0') {
        errno = ENOENT;
        return MIOS_NULL;
    }

    /* Punto de partida: siempre una ruta absoluta. */
    if (path[0] == '/') {
        out[0] = '\0';
        olen = 0;
        if (rp_copy(rest, path) < 0) {
            return MIOS_NULL;
        }
    } else {
        if (getcwd(out, PATH_MAX) == MIOS_NULL) {
            return MIOS_NULL;
        }
        olen = (int)mios_strlen(out);
        if (rp_copy(rest, path) < 0) {
            return MIOS_NULL;
        }
    }

    while (rest[rpos] != '\0') {
        int start;
        int clen;
        int n;

        while (rest[rpos] == '/') {     /* separadores de mas: se ignoran */
            rpos++;
        }
        if (rest[rpos] == '\0') {
            break;
        }
        start = rpos;
        while (rest[rpos] != '\0' && rest[rpos] != '/') {
            rpos++;
        }
        clen = rpos - start;

        if (clen == 1 && rest[start] == '.') {
            continue;                   /* "." no cambia nada */
        }
        if (clen == 2 && rest[start] == '.' && rest[start + 1] == '.') {
            olen = rp_up(out, olen);    /* ".." sube un nivel */
            continue;
        }

        mios_memcpy(comp, rest + start, (mios_size_t)clen);
        comp[clen] = '\0';

        n = 0;
        if (olen > 0) {
            mios_memcpy(cand, out, (mios_size_t)olen);
            n = olen;
        }
        cand[n] = '/';
        mios_memcpy(cand + n + 1, comp, (mios_size_t)clen + 1);

        if (mios_ret(mios_lstat(cand, &st)) < 0) {
            return MIOS_NULL;           /* no existe: errno ya esta puesto */
        }

        if ((st.st_mode & MIOS_S_IFMT) != MIOS_S_IFLNK) {
            olen = rp_append(out, olen, comp, clen);
            if (olen < 0) {
                return MIOS_NULL;
            }
            continue;
        }

        /* Es un enlace: se resuelve y se reanuda con su destino delante. */
        links++;
        if (links > RP_MAXLINKS) {
            errno = ELOOP;
            return MIOS_NULL;
        }

        {
            long got = mios_readlink(cand, link, (mios_size_t)PATH_MAX - 1);

            if (mios_ret(got) < 0) {
                return MIOS_NULL;
            }
            link[got] = '\0';           /* readlink() no termina la cadena */
        }
        {
            int llen = (int)mios_strlen(link);

            n = (int)mios_strlen(rest + rpos);

            if (llen + n + 1 > PATH_MAX) {
                errno = ENAMETOOLONG;
                return MIOS_NULL;
            }
            mios_memcpy(nuevo, link, (mios_size_t)llen);
            /* rest + rpos empieza por '/' o por '\0'. */
            mios_memcpy(nuevo + llen, rest + rpos, (mios_size_t)n + 1);
        }
        if (link[0] == '/') {           /* destino absoluto: de cero */
            olen = 0;
            out[0] = '\0';
        }
        mios_memcpy(rest, nuevo, mios_strlen(nuevo) + 1);
        rpos = 0;
    }

    if (olen == 0) {                    /* la raiz */
        out[0] = '/';
        out[1] = '\0';
        olen = 1;
    }

    if (resolved != MIOS_NULL) {
        mios_memcpy(resolved, out, (mios_size_t)olen + 1);
        return resolved;
    }

    r = malloc((mios_size_t)olen + 1);
    if (r == MIOS_NULL) {
        return MIOS_NULL;
    }
    mios_memcpy(r, out, (mios_size_t)olen + 1);
    return r;
}

/* Extension de GNU: como realpath(), pero reservando con malloc(). */
char *canonicalize_file_name(const char *path) {
    return realpath(path, MIOS_NULL);
}
