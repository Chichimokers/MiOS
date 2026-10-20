#include "lib/lib.h"
#include "kernel/syscall.h"
#include "../../include/mios.h"
#include "../../include/locale.h"

/*
 * MiOS - localizacion (locale.h).
 *
 * MiOS solo tiene la localizacion "C": no hay ficheros de locale, ni juegos de
 * caracteres alternativos, ni formato de numeros distinto.  Fase 7: binutils
 * llama a setlocale() al arrancar y necesita que le contesten algo.
 */

/* Los valores de la localizacion "C" (como en glibc: separadores vacios y
 * CHAR_MAX en los campos numericos). */
static struct lconv g_lconv_c = {
    ".", "", "", "", "", "", "", "", "", "",
    CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX,
    CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX,
    CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX
};

static char g_locale_c[] = "C";

/* Compara ignorando mayusculas (locale es minusculo en la practica). */
static int locale_eq(const char *name, const char *conocido) {
    while (*name != '\0' && *conocido != '\0') {
        char a = *name;
        char b = *conocido;
        if (a >= 'A' && a <= 'Z') {
            a = (char)(a - 'A' + 'a');
        }
        if (b >= 'A' && b <= 'Z') {
            b = (char)(b - 'A' + 'a');
        }
        if (a != b) {
            return 0;
        }
        name++;
        conocido++;
    }
    return *name == '\0' && *conocido == '\0';
}

char *setlocale(int category, const char *locale) {
    (void)category;

    if (locale == MIOS_NULL) {
        return g_locale_c;              /* consulta: la localizacion actual */
    }
    if (locale[0] == '\0') {
        return g_locale_c;              /* "" = la del entorno; en MiOS, "C" */
    }
    if (locale_eq(locale, "C") || locale_eq(locale, "POSIX")) {
        return g_locale_c;
    }
    /* Variantes tipo "C.UTF-8": siguen siendo "C". */
    if ((locale[0] == 'C' || locale[0] == 'c') && locale[1] == '.') {
        return g_locale_c;
    }
    return MIOS_NULL;                   /* no hay mas localizaciones */
}

struct lconv *localeconv(void) {
    return &g_lconv_c;
}
