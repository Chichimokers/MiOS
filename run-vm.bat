@echo off
rem ============================================================
rem  run-vm.bat - Arranca la VM de MiOS en VMware Workstation
rem
rem  La VM monta la ISO desde %USERPROFILE%\Downloads\MiOS.iso,
rem  asi que primero genera/trae la ISO con:  build-vps.bat
rem
rem  Si tu .vmx esta en otra ruta, edita VMFILE aqui abajo.
rem ============================================================

setlocal
set "VMFILE=%USERPROFILE%\Documents\Virtual Machines\Ubuntu\Ubuntu.vmx"

set "VMRUN=C:\Program Files (x86)\VMware\VMware Workstation\vmrun.exe"
if not exist "%VMRUN%" set "VMRUN=C:\Program Files\VMware\VMware Workstation\vmrun.exe"
if not exist "%VMRUN%" (
    echo [ERROR] No encuentro vmrun.exe de VMware Workstation.
    exit /b 1
)
if not exist "%VMFILE%" (
    echo [ERROR] No encuentro la VM: "%VMFILE%"
    echo         Edita la variable VMFILE en run-vm.bat.
    exit /b 1
)

echo [vm] Arrancando: "%VMFILE%"
"%VMRUN%" -T ws start "%VMFILE%"
if errorlevel 1 (
    echo [ERROR] vmrun no pudo arrancar la VM ^(puede que ya este encendida^).
    exit /b 1
)

echo [ok] VM arrancada. Entra en la consola MiOS y escribe 'help'.
echo [pista] Para el log serie de la VM: %USERPROFILE%\mioos.log
exit /b 0
