#include "lib/lib.h"
#include "kernel/syscall.h"
#include "../../include/mios.h"
#include "../../include/wctype.h"

/*
 * MiOS - clasificacion de caracteres anchos (wctype.h).
 *
 * Como en wchar.c, un caracter ancho es un byte.  Solo se convierten las
 * mayusculas/minusculas ASCII (la localizacion "C"); el resto se devuelve tal
 * cual.  Lo pide bfd al comparar los nombres de los recursos PE: towlower().
 */

wint_t towlower(wint_t c) {
    if (c >= (wint_t)'A' && c <= (wint_t)'Z') {
        return (wint_t)(c - (wint_t)'A' + (wint_t)'a');
    }
    return c;
}

wint_t towupper(wint_t c) {
    if (c >= (wint_t)'a' && c <= (wint_t)'z') {
        return (wint_t)(c - (wint_t)'a' + (wint_t)'A');
    }
    return c;
}
