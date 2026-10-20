@echo off
rem ============================================================
rem  pull-vps.bat - Trae a esta carpeta lo que hay en el VPS
rem                  (VPS -> Windows).  El sentido contrario es
rem                  sync-vps.bat (Windows -> VPS).
rem
rem    VPS     : 192.168.1.87   (usuario: chichimokers)
rem    Origen  : /home/chichimokers/OS Nuevo test   (con espacios)
rem
rem  Uso:  doble clic, o desde la consola:  pull-vps.bat
rem
rem  Que trae: TODO el arbol del repo (fuentes, docs, scripts y los
rem  artefactos del build: initramfs/init, initramfs/bin, initramfs/lib,
rem  initramfs/usr, iso/...), sobrescribiendo lo que haya aqui.
rem
rem  Que NO trae (a proposito):
rem    third_party/   fuentes descargadas (se regeneran con tools/fetch-*.sh)
rem    build/         objetos intermedios del build
rem    sysroot/       sysroot generado por mk-sysroot.sh
rem    mios-bin/      binarios sueltos de mios-cc.bat
rem    MiOS.iso       usad get-iso.bat (no pisar la ISO si la VM la tiene montada)
rem    *.cpio.gz      initramfs empaquetado (ya viene initramfs/ entero)
rem    .git/          historial; se queda el de Windows
rem  Ojo: lo que se haya borrado AQUI y siga en el VPS, vuelve.  Si borras
rem  algo de verdad, borralo tambien en el VPS (o no lances este script).
rem ============================================================

setlocal
cd /d "%~dp0"

set "VPS_HOST=192.168.1.87"
set "VPS_USER=chichimokers"
set "VPS_PASS=123"
set "VPS_DIR=/home/chichimokers/OS Nuevo test"
set "DEST=%VPS_USER%@%VPS_HOST%"

where ssh >nul 2>&1 || (echo [ERROR] No encuentro ssh.exe en el PATH. & exit /b 1)
where tar >nul 2>&1 || (echo [ERROR] No encuentro tar.exe en el PATH. & exit /b 1)

echo [pull] Origen : %DEST%:"%VPS_DIR%"
echo [pull] Destino: %CD%

rem --- 1) Intentar con clave SSH (sin contrasena) ---
set "SSH_OPTS=-o BatchMode=yes -o StrictHostKeyChecking=accept-new -o LogLevel=ERROR -o ConnectTimeout=8"
ssh %SSH_OPTS% %DEST% "echo ok" >nul 2>&1
if not errorlevel 1 goto keyok

rem --- 2) Respaldo: contrasena por SSH_ASKPASS ---
echo [info] Clave SSH no disponible; usando contrasena (SSH_ASKPASS)...
set "ASKPASS=%TEMP%\mios-askpass.cmd"
>"%ASKPASS%" echo @echo %VPS_PASS%
set "SSH_ASKPASS=%ASKPASS%"
set "SSH_ASKPASS_REQUIRE=force"
set "DISPLAY=localhost:0"
set "SSH_OPTS=-o StrictHostKeyChecking=accept-new -o LogLevel=ERROR -o ConnectTimeout=8"

:keyok
echo [pull] Trayendo (tar sobre ssh)...
rem  -h                 : los enlaces simbolicos se guardan como fichero real
rem  --hard-dereference : y los hard links, como copias (Windows no los necesita)
rem  --ignore-failed-read: si algo no se puede leer (p. ej. ficheros de root), sigue
ssh %SSH_OPTS% %DEST% "cd '%VPS_DIR%' && tar -chzf - --hard-dereference --ignore-failed-read --exclude='./.git' --exclude='./build' --exclude='./sysroot' --exclude='./mios-bin' --exclude='./third_party' --exclude='./MiOS.iso' --exclude='./*.cpio.gz' --exclude='./initramfs/dev' --exclude='./initramfs/tmp' ." | tar -xzf - -C .

if errorlevel 1 (
    echo [ERROR] La traida fallo.
    exit /b 1
)

echo [ok] Traido desde %DEST%:"%VPS_DIR%"
echo [recordatorio] Para el sentido Windows a VPS: sync-vps.bat (sube y despues trae).
exit /b 0
