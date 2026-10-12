# MiOS: crear una ISO arrancable con un `init.c` estático

## Objetivo

Construir una ISO de MiOS que arranque el kernel Linux y ejecute un
programa propio escrito en C como primer proceso de espacio de usuario
(`/init`). El programa imprime un mensaje como:

``` text
MiOS - Hola, este es mi sistema operativo ejecutando código C.
```

El flujo es:

``` text
src/init.c
   │ gcc -static
   ▼
initramfs/init                 (ejecutable ELF estático)
   │ cpio + gzip
   ▼
boot/initramfs.cpio.gz
   +
boot/vmlinuz                   (kernel Linux bzImage copiado con ese nombre)
   +
boot/grub/grub.cfg
   │ grub-mkrescue + xorriso
   ▼
MiOS.iso
```

Esta guía se concentra en ese método. **No utiliza BusyBox ni una shell
dentro del initramfs.**

## 1. Herramientas necesarias

En Debian, Ubuntu o un entorno Codespaces basado en Ubuntu:

``` bash
sudo apt update
sudo apt install -y build-essential binutils cpio gzip grub-common grub-pc-bin grub-efi-amd64-bin xorriso mtools
```

Comprueba las herramientas:

``` bash
gcc --version
cpio --version
gzip --version
grub-mkrescue --version
```

Los comandos `grub-mkrescue` y `xorriso` se usan para generar la ISO
arrancable.

## 2. Estructura del proyecto

En esta guía se usan las rutas de trabajo que aparecieron durante el
proyecto:

-   Proyecto MiOS: `~/MiOS`
-   Carpeta de preparación de la ISO: `/workspaces/codespaces-blank/iso`

Si tu proyecto está en otra ruta, sustituye las rutas por las reales.

Crea la estructura:

``` bash
mkdir -p ~/MiOS/src
mkdir -p ~/MiOS/initramfs/dev
mkdir -p /workspaces/codespaces-blank/iso/boot/grub
```

Al final, la carpeta `iso` debe quedar así:

``` text
iso/
└── boot/
    ├── vmlinuz
    ├── initramfs.cpio.gz
    └── grub/
        └── grub.cfg
```

La carpeta `~/MiOS/initramfs` será la raíz del sistema de archivos
inicial que Linux descomprime en memoria.

## 3. Escribir el programa `init.c`

Crea `~/MiOS/src/init.c`:

``` bash
cat > ~/MiOS/src/init.c <<'EOF'
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main(void)
{
    /*
     * Redirige la entrada, salida y error estándar a la consola del kernel.
     * El nodo /dev/console se incluye en el initramfs.
     */
    int console = open("/dev/console", O_RDWR);

    if (console >= 0) {
        dup2(console, STDIN_FILENO);
        dup2(console, STDOUT_FILENO);
        dup2(console, STDERR_FILENO);

        if (console > STDERR_FILENO) {
            close(console);
        }
    }

    puts("");
    puts("==============================================");
    puts("MiOS - Hola, este es mi sistema operativo.");
    puts("Mi primer proceso de usuario esta escrito en C.");
    puts("==============================================");
    fflush(stdout);

    /*
     * Mantiene vivo el proceso PID 1 para que el kernel no termine
     * porque el proceso init haya salido.
     */
    for (;;) {
        pause();
    }

    return 0;
}
EOF
```

Este programa se ejecuta como PID 1. No proporciona una terminal
interactiva: únicamente imprime el mensaje y queda esperando señales.

## 4. Compilar `init.c` como ejecutable estático

Ejecuta:

``` bash
gcc -static -O2 -Wall -Wextra -o ~/MiOS/initramfs/init ~/MiOS/src/init.c
```

La opción `-static` enlaza las bibliotecas necesarias dentro del
ejecutable, de modo que no necesite bibliotecas compartidas de un
sistema raíz que todavía no existe.

Comprueba el resultado:

``` bash
file ~/MiOS/initramfs/init
```

La salida debe identificar un ejecutable ELF y, normalmente, indicar que
está enlazado estáticamente. También puedes comprobar las dependencias:

``` bash
ldd ~/MiOS/initramfs/init
```

Si `ldd` dice que el ejecutable no es dinámico, es una señal esperada
para esta compilación estática. Si `gcc -static` falla porque faltan
bibliotecas estáticas, instala el paquete de desarrollo de libc de tu
distribución, por ejemplo:

``` bash
sudo apt install -y libc6-dev
```

No continúes hasta que el archivo `~/MiOS/initramfs/init` exista y sea
ejecutable.

## 5. Crear los archivos mínimos del initramfs

Crea las carpetas necesarias:

``` bash
mkdir -p ~/MiOS/initramfs/dev
```

El programa abre `/dev/console`. Para proporcionarle el dispositivo de
consola, en un entorno Linux con permisos suficientes ejecuta:

``` bash
sudo mknod -m 600 ~/MiOS/initramfs/dev/console c 5 1
```

Si el nodo ya existe, no hace falta crearlo de nuevo. Comprueba:

``` bash
ls -l ~/MiOS/initramfs/dev/console
```

En algunos contenedores o entornos Codespaces, `mknod` está restringido.
No lo sustituyas por un archivo normal vacío: un archivo normal no es un
dispositivo de consola. Si no puedes crear el nodo, utiliza un entorno
con permisos apropiados o adapta el programa y la configuración de
consola para el entorno de prueba.

Asegura los permisos del ejecutable:

``` bash
chmod 755 ~/MiOS/initramfs/init
```

La raíz mínima que vamos a empaquetar debe contener:

``` text
initramfs/
├── init
└── dev/
    └── console
```

No debe quedar un directorio `initramfs/` adicional dentro del archivo
CPIO. Al extraerlo, `init` debe estar directamente en la raíz.

## 6. Empaquetar el initramfs en CPIO y comprimirlo con gzip

Ejecuta los comandos exactamente desde la raíz de `initramfs`:

``` bash
cd ~/MiOS/initramfs
find . -print0 | cpio --null -ov --format=newc | gzip -9 > ~/MiOS/initramfs.cpio.gz
```

Qué hace cada parte:

-   `find . -print0`: enumera los archivos y directorios, incluidos los
    nombres con espacios.
-   `cpio --null -ov --format=newc`: crea el archivo CPIO en formato
    `newc`, utilizado para initramfs de Linux.
-   `gzip -9`: comprime el archivo.
-   `> ~/MiOS/initramfs.cpio.gz`: guarda el resultado.

Comprueba el archivo:

``` bash
ls -lh ~/MiOS/initramfs.cpio.gz
file ~/MiOS/initramfs.cpio.gz
gzip -t ~/MiOS/initramfs.cpio.gz
gzip -dc ~/MiOS/initramfs.cpio.gz | cpio -t
```

En el listado de CPIO debes ver `init` y `dev/console` en la raíz. La
primera ruta puede aparecer como `.` o `./`; eso es normal. **No debe
aparecer `MiOS/initramfs/init` como ruta interna.**

## 7. Copiar el kernel y el initramfs a la carpeta ISO

En el proyecto, el kernel compilado se encontró como:

``` text
arch/x86/boot/bzImage
```

Desde la raíz del árbol de fuentes del kernel Linux, copia ese archivo a
la carpeta de la ISO:

``` bash
cp arch/x86/boot/bzImage /workspaces/codespaces-blank/iso/boot/vmlinuz
```

El nombre `vmlinuz` es simplemente el nombre que le damos al archivo
dentro de la ISO; el artefacto fuente sigue siendo el `bzImage`
compilado.

Copia el initramfs:

``` bash
cp ~/MiOS/initramfs.cpio.gz /workspaces/codespaces-blank/iso/boot/initramfs.cpio.gz
```

Comprueba los dos archivos:

``` bash
ls -lh /workspaces/codespaces-blank/iso/boot/vmlinuz
ls -lh /workspaces/codespaces-blank/iso/boot/initramfs.cpio.gz
```

Si el árbol del kernel está en otra carpeta, ejecuta el primer `cp`
desde la raíz real de ese árbol o escribe la ruta absoluta de
`arch/x86/boot/bzImage`.

## 8. Crear la configuración GRUB

Para una ISO generada con `grub-mkrescue`, la configuración convencional
es:

``` text
iso/boot/grub/grub.cfg
```

Crea el archivo:

``` bash
cat > /workspaces/codespaces-blank/iso/boot/grub/grub.cfg <<'EOF'
set timeout=5
set default=0

menuentry "MiOS" {
    set root=(cd)
    linux /boot/vmlinuz
    initrd /boot/initramfs.cpio.gz
}
EOF
```

Comprueba el contenido:

``` bash
cat /workspaces/codespaces-blank/iso/boot/grub/grub.cfg
```

La línea `linux` carga el kernel; la línea `initrd` carga el archivo
CPIO comprimido que el kernel usará como sistema raíz inicial.

### Si GRUB cae en el prompt `grub>`

Durante las pruebas del proyecto se llegó al prompt de GRUB y se intentó
cargar manualmente `grub.conf`. Para esta configuración, el nombre que
debemos usar es `grub.cfg`:

``` text
set root=(cd)
ls (cd)/boot/
ls (cd)/boot/grub/
configfile (cd)/boot/grub/grub.cfg
```

Si la carga manual funciona, pero el arranque automático sigue cayendo
en `grub>`, comprueba que estés editando el archivo correcto y vuelve a
generar la ISO después de los cambios. `grub.conf` y `grub.cfg` son
nombres distintos; GRUB no los intercambia automáticamente.

## 9. Crear la ISO arrancable

Desde el directorio padre de `iso`:

``` bash
cd /workspaces/codespaces-blank
grub-mkrescue -o MiOS.iso iso
```

También se puede indicar la ruta absoluta:

``` bash
grub-mkrescue -o /workspaces/codespaces-blank/MiOS.iso /workspaces/codespaces-blank/iso
```

Comprueba que se generó:

``` bash
ls -lh /workspaces/codespaces-blank/MiOS.iso
file /workspaces/codespaces-blank/MiOS.iso
```

Si `grub-mkrescue` no existe, revisa la instalación de `grub-common`,
`grub-pc-bin`, `grub-efi-amd64-bin` y `xorriso`.

**Importante:** después de cambiar `init.c`, volver a compilar `init`,
regenerar el CPIO o modificar `grub.cfg`, hay que reconstruir la ISO. La
ISO existente no se actualiza al cambiar los archivos de la carpeta
`iso`.

## 10. Comprobación final antes del arranque

Ejecuta:

``` bash
test -x ~/MiOS/initramfs/init && echo "OK: init existe y es ejecutable"
gzip -t ~/MiOS/initramfs.cpio.gz && echo "OK: initramfs comprimido"
test -s /workspaces/codespaces-blank/iso/boot/vmlinuz && echo "OK: kernel copiado"
test -s /workspaces/codespaces-blank/iso/boot/initramfs.cpio.gz && echo "OK: initramfs copiado"
test -s /workspaces/codespaces-blank/iso/boot/grub/grub.cfg && echo "OK: configuración GRUB presente"
file /workspaces/codespaces-blank/MiOS.iso
```

Para confirmar qué contiene exactamente el initramfs que se incluirá en
la ISO:

``` bash
gzip -dc /workspaces/codespaces-blank/iso/boot/initramfs.cpio.gz | cpio -t
```

Debes ver `init` y `dev/console`.

## 11. Flujo de reconstrucción rápido

Una vez creado `init.c`, configurado GRUB y compilado el kernel, este es
el flujo para volver a construir todo después de cambiar el programa C.
Ejecuta el primer comando desde la raíz del árbol de fuentes del kernel:

``` bash
# 1. Compilar el programa C estático
gcc -static -O2 -Wall -Wextra -o ~/MiOS/initramfs/init ~/MiOS/src/init.c
chmod 755 ~/MiOS/initramfs/init

# 2. Empaquetar el initramfs
cd ~/MiOS/initramfs
find . -print0 | cpio --null -ov --format=newc | gzip -9 > ~/MiOS/initramfs.cpio.gz

# 3. Copiar el kernel (ejecutar desde la raíz de las fuentes Linux)
cp arch/x86/boot/bzImage /workspaces/codespaces-blank/iso/boot/vmlinuz

# 4. Copiar el initramfs
cp ~/MiOS/initramfs.cpio.gz /workspaces/codespaces-blank/iso/boot/initramfs.cpio.gz

# 5. Regenerar la ISO
cd /workspaces/codespaces-blank
grub-mkrescue -o MiOS.iso iso

# 6. Verificar la ISO
file MiOS.iso
ls -lh MiOS.iso
```

El bloque presupone que ya existe `~/MiOS/initramfs/dev/console` y que
`iso/boot/grub/grub.cfg` tiene la entrada de MiOS.

## 12. Qué debe ocurrir al arrancar

Si el kernel y la configuración son compatibles, GRUB cargará
`/boot/vmlinuz` y `/boot/initramfs.cpio.gz`. Linux descomprimirá el
initramfs y ejecutará `/init` como PID 1. El programa debe mostrar el
mensaje de MiOS y permanecer activo en el bucle `pause()`.

Si el kernel informa `No working init found` o
`Failed to execute /init`, comprueba:

1.  Que el archivo interno se llame exactamente `init` y esté en la raíz
    del CPIO.
2.  Que `init` tenga permisos de ejecución.
3.  Que `file init` indique un ejecutable para la arquitectura del
    kernel.
4.  Que el ejecutable sea estático.
5.  Que el kernel y el ejecutable tengan la misma arquitectura.
6.  Que `/dev/console` sea un nodo de dispositivo válido si el programa
    depende de él.

Si GRUB vuelve al prompt antes de que aparezca el mensaje de Linux, el
problema está en la carga de GRUB o en las rutas del kernel/initramfs,
no en la impresión del programa C. Si Linux empieza a arrancar pero no
ejecuta `/init`, revisa el contenido y los permisos del initramfs.

## Resultado

El resultado final es `MiOS.iso`, una ISO que contiene el kernel Linux,
el initramfs mínimo con el programa C estático y la configuración de
GRUB. El programa propio se ejecuta como `/init`; no se instala ni se
utiliza BusyBox.
