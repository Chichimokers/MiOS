@echo off
rem ============================================================
rem  get-iso.bat - Copia la MiOS.iso del VPS a este PC
rem
rem    Origen  : chichimokers@192.168.1.87:"/home/chichimokers/OS Nuevo test/MiOS.iso"
rem    Destino : %USERPROFILE%\Downloads\MiOS.iso   (sobrescribe la vieja)
rem
rem  Uso: doble clic, o desde la consola:  get-iso.bat
rem  No pide contrasena (clave SSH ya instalada).
rem ============================================================

setlocal
set "VPS_HOST=192.168.1.87"
set "VPS_USER=chichimokers"
set "VPS_ISO=/home/chichimokers/OS Nuevo test/MiOS.iso"
set "OUT=%USERPROFILE%\Downloads\MiOS.iso"
set "DEST=%VPS_USER%@%VPS_HOST%"
set "SSHOPT=-o BatchMode=yes -o StrictHostKeyChecking=accept-new -o LogLevel=ERROR"

where scp >nul 2>&1 || (echo [ERROR] No encuentro scp.exe en el PATH. & exit /b 1)

echo [iso] Origen : %DEST%:"%VPS_ISO%"
echo [iso] Destino: %OUT%
echo.

if exist "%OUT%" (
    echo [iso] La ISO actual se guarda como MiOS.iso.bak
    copy /y "%OUT%" "%OUT%.bak" >nul
)

echo [iso] Copiando (scp)...
scp %SSHOPT% "%DEST%:%VPS_ISO%" "%OUT%"
if errorlevel 1 (
    echo [iso] scp fallo; probando con ssh + cat...
    ssh %SSHOPT% %DEST% "cat '%VPS_ISO%'" > "%OUT%"
    if errorlevel 1 goto fail
)

echo.
echo [ok] Copiada a %OUT%
dir "%OUT%" | findstr /i "MiOS.iso"
echo.
echo [md5] REMOTO:
ssh %SSHOPT% %DEST% "md5sum '%VPS_ISO%'"
echo [md5] LOCAL :
certutil -hashfile "%OUT%" MD5 | findstr /r /v "hash CertUtil"
echo.
echo [recordatorio] Cierra/reinicia la VM para que monte la ISO nueva.
exit /b 0

:fail
echo.
echo [ERROR] No se pudo copiar la ISO.
exit /b 1
