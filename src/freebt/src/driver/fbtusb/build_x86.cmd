@echo off
call ..\..\..\..\local.cmd

set CURRENT_PATH=
for /f "tokens=* delims=" %%a in ('cd') do set CURRENT_PATH="%%a"

if "%CURRENT_PATH%" == "" (
   echo Error getting current path...
   goto exit
)

call %DDK_ROOT%\bin\setenv.bat %DDK_ROOT% %1 %2
pushd %CURRENT_PATH%
nmake
copy /y fbtusb.inf obj%1_%2_x86\i386\

:exit
