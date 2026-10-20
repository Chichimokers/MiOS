#include "mios.h"

/*
 * MiOS - traduccion de errores del kernel a mensajes legibles y errno.
 */

/* Almacenamiento de errno. Se expone con __errno_location() (estilo glibc)
 * para no depender de una variable global importada de la .so. */
static int mios_errno_storage = 0;

int *__errno_location(void) {
    return &mios_errno_storage;
}

const char *mios_strerror(int err) {
    switch (err) {
    case 0:
        return "sin error";
    case MIOS_EPERM:
        return "operacion no permitida";
    case MIOS_ENOENT:
        return "no existe";
    case MIOS_ESRCH:
        return "no hay tal proceso";
    case MIOS_EINTR:
        return "llamada interrumpida";
    case MIOS_EIO:
        return "error de entrada/salida";
    case MIOS_ENXIO:
        return "dispositivo inexistente";
    case MIOS_E2BIG:
        return "lista de argumentos demasiado larga";
    case MIOS_ENOEXEC:
        return "formato ejecutable invalido";
    case MIOS_EBADF:
        return "descriptor invalido";
    case MIOS_ECHILD:
        return "no hay procesos hijos";
    case MIOS_EAGAIN:
        return "recurso no disponible";
    case MIOS_ENOMEM:
        return "sin memoria";
    case MIOS_EACCES:
        return "permiso denegado";
    case MIOS_EFAULT:
        return "direccion invalida";
    case MIOS_EBUSY:
        return "recurso ocupado";
    case MIOS_EEXIST:
        return "ya existe";
    case MIOS_EXDEV:
        return "dispositivo cruzado";
    case MIOS_ENODEV:
        return "no existe el dispositivo";
    case MIOS_ENOTDIR:
        return "no es un directorio";
    case MIOS_EISDIR:
        return "es un directorio";
    case MIOS_EINVAL:
        return "argumento invalido";
    case MIOS_ENFILE:
        return "demasiados archivos abiertos en el sistema";
    case MIOS_EMFILE:
        return "demasiados archivos abiertos";
    case MIOS_ENOTTY:
        return "no es un terminal";
    case MIOS_ENOSPC:
        return "no queda espacio";
    case MIOS_ESPIPE:
        return "busqueda invalida";
    case MIOS_EROFS:
        return "sistema de archivos de solo lectura";
    case MIOS_EPIPE:
        return "tuberia rota";
    case MIOS_EDOM:
        return "argumento fuera del dominio";
    case MIOS_ERANGE:
        return "resultado fuera de rango";
    case MIOS_ENAMETOOLONG:
        return "nombre demasiado largo";
    case MIOS_ENOSYS:
        return "funcion no implementada";
    case MIOS_ENOTEMPTY:
        return "directorio no vacio";
    case MIOS_EILSEQ:
        return "secuencia de bytes invalida";
    case MIOS_EOVERFLOW:
        return "valor demasiado grande";
    case MIOS_ENOTSUP:
        return "operacion no soportada";
    default:
        return "error desconocido";
    }
}
