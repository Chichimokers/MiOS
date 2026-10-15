#include "mios.h"

/*
 * MiOS - traduccion de errores del kernel a mensajes legibles.
 */

const char *mios_strerror(int err) {
    switch (err) {
    case 0:
        return "sin error";
    case MIOS_EPERM:
        return "operacion no permitida";
    case MIOS_ENOENT:
        return "no existe";
    case MIOS_EIO:
        return "error de entrada/salida";
    case MIOS_ENXIO:
        return "dispositivo inexistente";
    case MIOS_EBADF:
        return "descriptor invalido";
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
    case MIOS_ENOTDIR:
        return "no es un directorio";
    case MIOS_EISDIR:
        return "es un directorio";
    case MIOS_EINVAL:
        return "argumento invalido";
    case MIOS_EMFILE:
        return "demasiados archivos abiertos";
    case MIOS_ENOSPC:
        return "no queda espacio";
    case MIOS_EROFS:
        return "sistema de archivos de solo lectura";
    case MIOS_ENAMETOOLONG:
        return "nombre demasiado largo";
    case MIOS_ENOSYS:
        return "funcion no implementada";
    case MIOS_ENOTEMPTY:
        return "directorio no vacio";
    default:
        return "error desconocido";
    }
}
