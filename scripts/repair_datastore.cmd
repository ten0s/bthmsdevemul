@echo off
call ..\local.cmd
set CORECON_PATH=C:\Documents and Settings\%USERNAME%\Local Settings\Application Data\Microsoft\CoreCon
set CORECON_VER=1.0

echo Close all Visual Studio Instances...
pause

taskkill /F /IM dvcemumanager.exe

regsvr32 /s "%STUDIO2005_ROOT%\SmartDevices\Emulators\DeviceEmulator\DeviceEmulatorbootstrap.dll"

if exist "%CORECON_PATH%\%CORECON_VER%" (
   pushd "%CORECON_PATH%"
   ren %CORECON_VER% __%CORECON_VER%
   popd
   psexec -d "%PROGRAMFILES%\Microsoft Device Emulator\1.0\dvcemumanager.exe"
)

if exist "%CORECON_PATH%\__%CORECON_VER%" (
   pushd "%CORECON_PATH%"
   rd /s /q __%CORECON_VER%
   popd
)


echo If the above didn't help repair your Visual Studio installation:
echo (1) Control Panel
echo (2) Add/Remove Programs
echo (3) Visual Studio 2005
echo (4) Repair
pause




