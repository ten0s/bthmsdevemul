call ..\local.cmd
call "%STUDIO2005_ROOT%\VC\vcvarsall.bat" x86
set CONFIGURATION=%1

pushd ..\BthEmulManager
devenv BthEmulManager.sln /build "%CONFIGURATION%" /project fbtlib /projectconfig "%CONFIGURATION%|Win32"
devenv BthEmulManager.sln /build "%CONFIGURATION%" /project fbtrt /projectconfig "%CONFIGURATION%|Win32"
devenv BthEmulManager.sln /build "%CONFIGURATION%" /project bthemul /projectconfig "%CONFIGURATION%|STANDARDSDK_500 (ARMV4I)"
devenv BthEmulManager.sln /build "%CONFIGURATION%" /project bthemulcom /projectconfig "%CONFIGURATION%|STANDARDSDK_500 (ARMV4I)"
devenv BthEmulManager.sln /build "%CONFIGURATION%" /project BthEmulAgent /projectconfig "%CONFIGURATION%|STANDARDSDK_500 (ARMV4I)"
devenv BthEmulManager.sln /build "%CONFIGURATION%" /project BthEmulManagerPlugin /projectconfig "%CONFIGURATION%|Any CPU"
popd