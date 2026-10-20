@echo off
rem ============================================================
rem  build-vps.bat - Compila MiOS en el VPS y trae la ISO al PC
rem
rem   1) Sincroniza este repo con el VPS        (sync-vps.bat)
rem   2) Ejecuta ./build.sh dentro del VPS
rem   3) Trae los artefactos del build          (pull-vps.bat)
rem   4) Copia la ISO resultante a %USERPROFILE%\Downloads\MiOS.iso
rem
rem  VPS: chichimokers@192.168.1.87:/home/chichimokers/OS Nuevo test
rem
rem  Uso: doble clic, o desde la consola:  build-vps.bat
rem  No pide contrasena (clave SSH ya instalada).
rem ============================================================

setlocal
cd /d "%~dp0"

set "VPS_HOST=192.168.1.87"
set "VPS_USER=chichimokers"
set "VPS_DIR=/home/chichimokers/OS Nuevo test"
set "DEST=%VPS_USER%@%VPS_HOST%"
set "SSHOPT=-o BatchMode=yes -o StrictHostKeyChecking=accept-new -o LogLevel=ERROR"

echo ============================================================
echo  [1/4] Sincronizando con el VPS (subir y traer)...
echo ============================================================
call "%~dp0sync-vps.bat"
if errorlevel 1 (echo. & echo [ERROR] La sincronizacion fallo. & exit /b 1)

echo.
echo ============================================================
echo  [2/4] Compilando en el VPS (bash build.sh)...
echo ============================================================
ssh %SSHOPT% %DEST% "cd '%VPS_DIR%' && bash build.sh"
if errorlevel 1 (echo. & echo [ERROR] La compilacion fallo en el VPS. & exit /b 1)

echo.
echo ============================================================
echo  [3/4] Trayendo los artefactos del build (initramfs, etc.)...
echo ============================================================
call "%~dp0pull-vps.bat"
if errorlevel 1 (echo. & echo [ERROR] No se pudo traer del VPS. & exit /b 1)

echo.
echo ============================================================
echo  [4/4] Trayendo la ISO al PC...
echo ============================================================
call "%~dp0get-iso.bat"
if errorlevel 1 (echo. & echo [ERROR] No se pudo traer la ISO. & exit /b 1)

echo.
echo [ok] Ciclo completo. ISO nueva en %USERPROFILE%\Downloads\MiOS.iso
echo [siguiente] Arranca la VM con:  run-vm.bat
exit /b 0
