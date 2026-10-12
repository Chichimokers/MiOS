# MiOS: guía de construcción del kernel, initramfs e imagen ISO

Esta guía recoge el estado alcanzado en el proyecto **MiOS**: kernel
Linux compilado, BusyBox dentro del `initramfs`, arranque de prueba que
muestra `MiOS - Hello World!` y creación de una ISO arrancable con GRUB.

> **Estado conocido:** el arranque manual desde el prompt `grub>`
> funciona. El arranque automático todavía cae en `grub>`. Por eso, la
> guía separa la construcción de la ISO de la corrección pendiente de la
> configuración de GRUB.

## 1. Estructura del proyecto

Durante el trabajo se han usado dos directorios: `~/MiOS` para las
fuentes y el sistema raíz, y `iso/` para preparar la imagen. En
Codespaces se usó `/workspaces/codespaces-blank/iso`.

Una estructura de referencia es:

``` text
MiOS/
├── src/
│   └── init.c
├── initramfs/
│   ├── init
│   ├── bin/
│   │   ├── busybox
│   │   └── enlaces de BusyBox
│   ├── dev/
│   │   └── console
│   ├── proc/
│   ├── sys/
│   └── ...
└── linux/                 # árbol de fuentes del kernel, si se guardó aquí

iso/
└── boot/
    ├── vmlinuz
    ├── initramfs.cpio.gz
    └── grub/
        └── grub.cfg
```

Los nombres exactos del árbol de fuentes del kernel y la ubicación de la
carpeta de trabajo pueden variar. En los comandos siguientes, ajusta
`~/MiOS` y la ruta de las fuentes si tu copia está en otro sitio.

## 2. Kernel Linux

El kernel ya se compiló y el archivo generado que identificamos fue:

``` text
arch/x86/boot/bzImage
```

Desde la raíz del árbol de fuentes de Linux, una compilación típica es:

``` bash
make -j"$(nproc)"
```

Si aún no se ha configurado el kernel, primero hace falta una
configuración válida. Para una configuración inicial en x86:

``` bash
make x86_64_defconfig
make -j"$(nproc)"
```

No ejecutes `x86_64_defconfig` encima de una configuración personalizada
si quieres conservarla. Si el kernel ya está compilado, no hace falta
recompilarlo para volver a empaquetar la ISO.

Comprueba el artefacto:

``` bash
ls -lh arch/x86/boot/bzImage
file arch/x86/boot/bzImage
```

Cópialo a la carpeta de arranque de la ISO como `vmlinuz`:

``` bash
mkdir -p /workspaces/codespaces-blank/iso/boot/grub
cp arch/x86/boot/bzImage /workspaces/codespaces-blank/iso/boot/vmlinuz
```

Si trabajas en otra ubicación, sustituye
`/workspaces/codespaces-blank/iso` por la ruta real de tu carpeta `iso`.

## 3. BusyBox estático dentro del initramfs

Se utilizó BusyBox 1.37.0. La compilación debe producir un binario
**estático**, para no depender de bibliotecas compartidas que todavía no
existen dentro del sistema raíz inicial.

Desde el directorio de fuentes de BusyBox:

``` bash
make menuconfig
```

En la configuración de BusyBox activa:

``` text
Settings  --->
  [*] Build static binary (no shared libs)
```

Guarda la configuración y compila:

``` bash
make -j"$(nproc)"
```

Comprueba que sea estático:

``` bash
file busybox
ldd busybox
```

En un binario estático, `file` suele indicar que está enlazado
estáticamente y `ldd` puede indicar que no es dinámico o que no se puede
usar `ldd` con él.

Copia BusyBox al sistema raíz:

``` bash
mkdir -p ~/MiOS/initramfs/bin
cp ~/MiOS/busybox-1.37.0/busybox ~/MiOS/initramfs/bin/busybox
```

Instala los enlaces simbólicos de los applets de BusyBox:

``` bash
cd ~/MiOS/initramfs/bin
./busybox --install -s .
```

Verifica algunos comandos:

``` bash
ls -l ~/MiOS/initramfs/bin/sh
ls -l ~/MiOS/initramfs/bin/mount
```

Los enlaces apuntan al ejecutable `/bin/busybox`.

## 4. Crear el programa `/init`

El kernel busca `/init` en el initramfs como primer proceso de espacio
de usuario. Debe existir y ser ejecutable. En el proyecto se probaron
dos alternativas: un script de shell y un programa C estático. Usa **una
sola** como `/init` en cada prueba.

### Opción A: `/init` escrito en shell

Crea el archivo:

``` bash
cat > ~/MiOS/initramfs/init <<'EOF'
#!/bin/sh

mount -t devtmpfs devtmpfs /dev
mount -t proc proc /proc
mount -t sysfs sysfs /sys

echo
echo "MiOS - Hello World!"
echo "Sistema iniciado desde initramfs."
echo

exec /bin/sh
EOF

chmod +x ~/MiOS/initramfs/init
```

Esta opción necesita que BusyBox tenga habilitados `sh`, `mount` y los
comandos que quieras ejecutar. Si el kernel no monta `/dev`
automáticamente, el montaje de `devtmpfs` permite crear los dispositivos
disponibles; asegúrate también de que el kernel tenga soporte para
`devtmpfs`.

### Opción B: programa C estático

El proyecto también utilizó `src/init.c` para imprimir
`MiOS - Hello World!` y mantener el proceso vivo con `pause()`.

Ejemplo mínimo:

``` c
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

int main(void) {
    int console = open("/dev/console", O_RDWR);
    if (console >= 0) {
        dup2(console, STDIN_FILENO);
        dup2(console, STDOUT_FILENO);
        dup2(console, STDERR_FILENO);
        if (console > STDERR_FILENO) close(console);
    }

    puts("MiOS - Hello World!");
    fflush(stdout);

    for (;;) pause();
    return 0;
}
```

Compílalo estáticamente:

``` bash
gcc -static -O2 -Wall -o ~/MiOS/initramfs/init ~/MiOS/src/init.c
chmod +x ~/MiOS/initramfs/init
file ~/MiOS/initramfs/init
```

**Importante:** la versión C que solo imprime y hace `pause()` sirve
para comprobar que el kernel ejecuta `/init`, pero no crea una shell
interactiva. Para tener una consola de comandos, usa la opción shell o
modifica el programa C para montar los sistemas virtuales y lanzar una
shell.

## 5. Preparar los directorios y dispositivos del initramfs

Asegura la estructura básica:

``` bash
mkdir -p ~/MiOS/initramfs/{bin,dev,proc,sys}
```

Para la opción C que abre `/dev/console`, debe existir el nodo de
consola en el initramfs. En un entorno Linux con permisos adecuados:

``` bash
sudo mknod -m 600 ~/MiOS/initramfs/dev/console c 5 1
```

Si ya existe, no vuelvas a crearlo. Puedes comprobarlo con:

``` bash
ls -l ~/MiOS/initramfs/dev/console
```

`mknod` crea un dispositivo especial; no es un archivo normal. En
algunos entornos de contenedores o Codespaces no se permite crear nodos
de dispositivo. Si falla, no lo sustituyas por un archivo vacío: usa un
entorno con permisos adecuados o asegúrate de que el kernel monte
`devtmpfs` antes de abrir la consola.

Asegura los permisos del punto de entrada:

``` bash
chmod +x ~/MiOS/initramfs/init
```

## 6. Empaquetar el initramfs como `initramfs.cpio.gz`

El orden importa: entra en la raíz del initramfs para que las rutas
almacenadas en el archivo comiencen por `init`, `bin/`, `dev/`, etc., y
no por la ruta absoluta de tu máquina.

``` bash
cd ~/MiOS/initramfs
find . -print0 | cpio --null -ov --format=newc | gzip -9 > /tmp/initramfs.cpio.gz
```

Copia el archivo generado a la ISO:

``` bash
cp /tmp/initramfs.cpio.gz /workspaces/codespaces-blank/iso/boot/initramfs.cpio.gz
```

Comprueba el archivo:

``` bash
ls -lh /tmp/initramfs.cpio.gz
file /tmp/initramfs.cpio.gz
gzip -t /tmp/initramfs.cpio.gz
gzip -dc /tmp/initramfs.cpio.gz | cpio -t | head -40
```

El formato `newc` es uno de los formatos esperados para un initramfs
Linux. No empaquetes la carpeta contenedora `initramfs` completa como un
nivel adicional: el archivo debe contener `./init` en la raíz.

## 7. Preparar los archivos de la ISO

La carpeta que se entrega a `grub-mkrescue` debe contener al menos:

``` text
iso/
└── boot/
    ├── vmlinuz
    ├── initramfs.cpio.gz
    └── grub/
        └── grub.cfg
```

Verifica que los nombres coincidan exactamente:

``` bash
find /workspaces/codespaces-blank/iso -maxdepth 4 -type f -print
ls -lh /workspaces/codespaces-blank/iso/boot/
```

En el trabajo también apareció una carpeta llamada `mios-iso`; la
carpeta de preparación que se estaba usando finalmente se identificó
como `iso`. Ejecuta la creación de la ISO desde el directorio padre de
esa carpeta o especifica su ruta completa.

## 8. Configuración de GRUB para arrancar automáticamente

Para una ISO generada con `grub-mkrescue`, el archivo convencional es:

``` text
iso/boot/grub/grub.cfg
```

Contenido recomendado para esta estructura:

``` cfg
set timeout=5
set default=0

menuentry "MiOS" {
    set root=(cd)
    linux /boot/vmlinuz
    initrd /boot/initramfs.cpio.gz
}
```

En una ISO de GRUB, `(cd)` representa el dispositivo de CD/DVD desde el
que GRUB está leyendo. La entrada carga el kernel y, después, el
initramfs.

Comprueba el nombre y la ubicación del archivo:

``` bash
ls -l /workspaces/codespaces-blank/iso/boot/grub/grub.cfg
cat /workspaces/codespaces-blank/iso/boot/grub/grub.cfg
```

En las pruebas manuales, el comando que se llegó a usar fue:

``` text
configfile (cd)/boot/grub/grub.conf
```

Eso significa que se estaba intentando cargar **`grub.conf`**, no
`grub.cfg`. GRUB no convierte automáticamente un nombre en el otro. Para
el arranque automático de una ISO creada con `grub-mkrescue`, utiliza
`boot/grub/grub.cfg`. Si decides mantener `grub.conf`, tendrás que
cargarlo explícitamente desde otro archivo `grub.cfg`; no basta con
cambiar el nombre en el prompt y esperar que GRUB lo descubra solo.

### Diagnóstico desde el prompt `grub>`

Estos comandos fueron útiles para la prueba manual:

``` text
set root=(cd)
set prefix=(cd)/boot/grub
configfile (cd)/boot/grub/grub.conf
```

Si tu archivo definitivo se llama `grub.cfg`, prueba:

``` text
set root=(cd)
ls (cd)/boot/
ls (cd)/boot/grub/
configfile (cd)/boot/grub/grub.cfg
```

Si `configfile` muestra el menú o ejecuta la entrada, la configuración
se puede leer manualmente. Si la carga manual funciona pero el arranque
automático vuelve a `grub>`, el problema pendiente está en cómo GRUB
encuentra o ejecuta su configuración predeterminada, o en que el archivo
incluido en la ISO no es el mismo que estás editando.

## 9. Instalar herramientas y crear la ISO

En Debian/Ubuntu/Codespaces, instala las herramientas necesarias:

``` bash
sudo apt update
sudo apt install -y grub-common grub-pc-bin grub-efi-amd64-bin xorriso mtools
```

Según el entorno, no todos los paquetes de GRUB serán necesarios, pero
`grub-mkrescue` y `xorriso` son los principales para crear una ISO
arrancable.

Desde el directorio que contiene `iso/`:

``` bash
cd /workspaces/codespaces-blank
grub-mkrescue -o MiOS.iso iso
```

También puedes indicar rutas completas:

``` bash
grub-mkrescue -o /workspaces/codespaces-blank/MiOS.iso /workspaces/codespaces-blank/iso
```

Verifica el resultado:

``` bash
ls -lh /workspaces/codespaces-blank/MiOS.iso
file /workspaces/codespaces-blank/MiOS.iso
```

Si modificas `grub.cfg`, el kernel o el initramfs después de crear la
ISO, vuelve a ejecutar `grub-mkrescue`; editar la carpeta `iso/` no
modifica una ISO que ya existe.

## 10. Comprobaciones antes de probarla

Ejecuta:

``` bash
test -x /workspaces/codespaces-blank/iso/boot/vmlinuz && echo "Kernel presente"
test -s /workspaces/codespaces-blank/iso/boot/initramfs.cpio.gz && echo "Initramfs presente"
test -s /workspaces/codespaces-blank/iso/boot/grub/grub.cfg && echo "GRUB config presente"
gzip -t /workspaces/codespaces-blank/iso/boot/initramfs.cpio.gz
grep -nE 'menuentry|linux|initrd|set timeout|set default' /workspaces/codespaces-blank/iso/boot/grub/grub.cfg
```

`vmlinuz` es el nombre que se usó para copiar el archivo `bzImage`; el
nombre no cambia el formato del kernel.

En una de las comprobaciones de Windows, los archivos visibles dentro de
la carpeta de preparación tenían estos tamaños aproximados:

-   `boot/vmlinuz`: 15 030 664 bytes.
-   `boot/initramfs.cpio.gz`: 340 142 bytes.
-   `boot/grub/grub.cfg`: 150 bytes.

Esos tamaños son una referencia de aquella copia, no requisitos fijos.
Los tamaños cambiarán si recompilas el kernel, modificas BusyBox o
cambias `/init`.

## 11. Flujo completo resumido

Una vez compilados el kernel y BusyBox, el empaquetado y la creación de
la ISO se pueden repetir así:

``` bash
# 1. Copiar el kernel ya compilado (ejecutar desde el árbol de fuentes Linux)
cp arch/x86/boot/bzImage /workspaces/codespaces-blank/iso/boot/vmlinuz

# 2. Empaquetar el initramfs desde su raíz
cd ~/MiOS/initramfs
find . -print0 | cpio --null -ov --format=newc | gzip -9 > /tmp/initramfs.cpio.gz

# 3. Copiar el initramfs a la ISO
cp /tmp/initramfs.cpio.gz /workspaces/codespaces-blank/iso/boot/initramfs.cpio.gz

# 4. Comprobar el archivo comprimido
gzip -t /workspaces/codespaces-blank/iso/boot/initramfs.cpio.gz

# 5. Regenerar la ISO
cd /workspaces/codespaces-blank
grub-mkrescue -o MiOS.iso iso

# 6. Verificar el resultado
file MiOS.iso
ls -lh MiOS.iso
```

Este bloque presupone que `iso/boot/grub/grub.cfg` ya existe y contiene
la entrada de MiOS, y que las rutas usadas corresponden a tu entorno
actual.

## 12. Estado actual y problema pendiente

**Lo que ya se logró:**

-   Compilar el kernel Linux y localizar `arch/x86/boot/bzImage`.
-   Preparar BusyBox estático y copiarlo al initramfs.
-   Crear `/init` en shell o en C estático.
-   Empaquetar el initramfs con `cpio` en formato `newc` y comprimirlo
    con `gzip`.
-   Preparar `boot/vmlinuz` e `boot/initramfs.cpio.gz`.
-   Arrancar manualmente desde GRUB y llegar al mensaje
    `MiOS - Hello World!`.

**Lo que falta cerrar:**

-   Conseguir que GRUB lea automáticamente la configuración al arrancar
    la ISO, sin tener que escribir comandos en `grub>`.
-   Confirmar que la ISO final contiene `boot/grub/grub.cfg` con el
    contenido correcto.
-   Probar de nuevo la ISO regenerada después de cualquier cambio en
    GRUB.

La diferencia clave que se debe revisar primero es el nombre: durante el
diagnóstico se ejecutó `configfile .../grub.conf`, mientras que la
ubicación convencional para la configuración automática de una ISO
creada con `grub-mkrescue` es `boot/grub/grub.cfg`. Asegúrate de editar
el archivo correcto y de regenerar `MiOS.iso` después.
