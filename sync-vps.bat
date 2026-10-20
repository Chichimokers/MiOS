@echo off
rem ============================================================
rem  sync-vps.bat - Sincroniza esta carpeta con el VPS de build
rem
rem    VPS     : 192.168.1.87   (usuario: chichimokers)
rem    Destino : /home/chichimokers/OS Nuevo test   (con espacios)
rem
rem  Uso:  doble clic, o desde la consola:  sync-vps.bat
rem
rem  Lo hace en LOS DOS SENTIDOS:
rem    1) sube esta carpeta al VPS (fuentes, docs, scripts)
rem    2) y luego trae del VPS lo que haya cambiado alli (los artefactos
rem       del build: initramfs/init, initramfs/bin, lib, usr, iso/...)
rem  Asi los dos lados quedan iguales y Windows no se queda con una copia
rem  vieja de lo que se compilo en el VPS.  Se sube ANTES de traer, para no
rem  pisar ningun cambio local sin subir.
rem  Solo traer (sin subir): pull-vps.bat
rem
rem  No pide contrasena: se instalo una clave SSH una sola vez.
rem  Si la clave faltara, cae al respaldo con contrasena via
rem  SSH_ASKPASS (la 123).  plink/pscp no son necesarios.
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

echo [sync] Origen : %CD%
echo [sync] Destino: %DEST%:"%VPS_DIR%"

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
echo [sync] Enviando archivos (tar sobre ssh)...
rem Nota: initramfs/init se excluye a proposito. Es un artefacto que genera build.sh
rem (paso 3); el que hay en Windows es viejo y, si se copiara, pisaria el recien
rem compilado en el VPS y despistaria (p. ej. al mirar 'strings initramfs/init').
rem Lo mismo vale para el resto de artefactos del build (binarios de /bin, cabeceras
rem instaladas, bibliotecas y enlaces de /lib): los genera el VPS y el pull los trae
rem a Windows como espejo; subirlos de vuelta sustituiria los enlaces (libgmp.so ->
rem libgmp.so.10.4.1) por copias del fichero entero.
tar --exclude "./MiOS.iso" --exclude "./*.cpio.gz" --exclude "./initramfs/init" ^
    --exclude "./build" --exclude "./sysroot" --exclude "./mios-bin" ^
    --exclude "./initramfs/bin" --exclude "./initramfs/usr" --exclude "./initramfs/lib/tcc" ^
    --exclude "./initramfs/lib/libmios.so*" --exclude "./initramfs/lib/libm.so*" ^
    --exclude "./initramfs/lib/libc.so*" --exclude "./initramfs/lib/ld-mios.so.1" ^
    --exclude "./initramfs/lib/libgmp.so*" --exclude "./initramfs/lib/libmpfr.so*" ^
    --exclude "./initramfs/lib/libmpc.so*" ^
    -czf - . ^
    | ssh %SSH_OPTS% %DEST% "mkdir -p '%VPS_DIR%' && tar -xzf - -C '%VPS_DIR%' && (cd '%VPS_DIR%' && chmod +x *.sh mios-gcc tools/*.sh 2>/dev/null || true)"
rem Nota: el tar que crea Windows no conserva el bit +x, asi que se lo devolvemos a
rem los scripts en el VPS (si no, 'build.sh' y 'mios-gcc' fallan con Permission denied).

if errorlevel 1 (
    echo [ERROR] La sincronizacion fallo.
    exit /b 1
)

rem --- 3) y de vuelta: traer del VPS lo que haya cambiado alli ---
echo [sync] De vuelta: trayendo del VPS (artefactos del build)...
call "%~dp0pull-vps.bat"
if errorlevel 1 exit /b 1

echo [ok] Sincronizado con %DEST%:"%VPS_DIR%" (subido y traido)
echo [recordatorio] Compila y prueba en el VPS; las pruebas dentro del SO las hace el usuario.
exit /b 0
