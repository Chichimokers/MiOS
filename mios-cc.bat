@echo off
rem ============================================================
rem  mios-cc.bat - Compila un programa para MiOS con "mios-gcc"
rem               en el VPS y trae el binario a mios-bin\.
rem
rem  Uso:
rem    mios-cc.bat examples\hola.c            compila y trae el binario
rem    mios-cc.bat examples\hola.c hola       con otro nombre
rem    mios-cc.bat examples\hola.c hola run   ademas lo ejecuta en el VPS
rem
rem  VPS : 192.168.1.87  (chichimokers)
rem  No pide contrasena: hay clave SSH; si faltara, cae al respaldo
rem  con contrasena via SSH_ASKPASS (la 123).
rem ============================================================

setlocal
cd /d "%~dp0"

set "VPS_HOST=192.168.1.87"
set "VPS_USER=chichimokers"
set "VPS_PASS=123"
set "VPS_DIR=/home/chichimokers/OS Nuevo test"
set "DEST=%VPS_USER%@%VPS_HOST%"
set "EXCH=mios-cc"

where ssh >nul 2>&1 || (echo [ERROR] No encuentro ssh.exe en el PATH. & exit /b 1)
where scp >nul 2>&1 || (echo [ERROR] No encuentro scp.exe en el PATH. & exit /b 1)

set "SRC=%~1"
if "%SRC%"=="" (
    echo Uso: mios-cc.bat archivo.c [nombre] [run]
    exit /b 1
)
if not exist "%SRC%" (
    echo [ERROR] No existe el fichero "%SRC%".
    exit /b 1
)

set "NAME=%~2"
if "%NAME%"=="" set "NAME=%~n1"
for %%F in ("%SRC%") do set "BASE=%%~nxF"

rem --- Clave SSH (sin contrasena); si no, respaldo con contrasena ---
set "SSH_OPTS=-o BatchMode=yes -o StrictHostKeyChecking=accept-new -o LogLevel=ERROR -o ConnectTimeout=8"
ssh %SSH_OPTS% %DEST% "echo ok" >nul 2>&1
if not errorlevel 1 goto keyok

echo [info] Clave SSH no disponible; usando contrasena (SSH_ASKPASS)...
set "ASKPASS=%TEMP%\mios-askpass.cmd"
>"%ASKPASS%" echo @echo %VPS_PASS%
set "SSH_ASKPASS=%ASKPASS%"
set "SSH_ASKPASS_REQUIRE=force"
set "DISPLAY=localhost:0"
set "SSH_OPTS=-o StrictHostKeyChecking=accept-new -o LogLevel=ERROR -o ConnectTimeout=8"

:keyok
echo [cc] %SRC%  --^>  mios-bin\%NAME%   (via %DEST%)

ssh %SSH_OPTS% %DEST% "mkdir -p '%VPS_DIR%/%EXCH%' ~/%EXCH%" || goto fail
scp %SSH_OPTS% "%SRC%" %DEST%:%EXCH%/%BASE% || goto fail

echo [cc] Compilando con mios-gcc en el VPS...
ssh %SSH_OPTS% %DEST% "cd '%VPS_DIR%' && bash mios-gcc ~/%EXCH%/%BASE% -o ~/%EXCH%/%NAME%" || goto fail

if not exist "mios-bin" mkdir "mios-bin"
scp %SSH_OPTS% %DEST%:%EXCH%/%NAME% "mios-bin\%NAME%" || goto fail

echo [ok] Binario de MiOS: %CD%\mios-bin\%NAME%
ssh %SSH_OPTS% %DEST% "file ~/%EXCH%/%NAME% | cut -d, -f1-3"

if /i "%~3"=="run" (
    echo [cc] Ejecutando en el chroot del sysroot del VPS...
    ssh %SSH_OPTS% %DEST% "cd '%VPS_DIR%' && cp -f ~/%EXCH%/%NAME% sysroot/%NAME% && echo %VPS_PASS% | sudo -S chroot sysroot /%NAME%" || goto fail
)

exit /b 0

:fail
echo [ERROR] La compilacion fallo.
exit /b 1
