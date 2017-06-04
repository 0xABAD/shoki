@echo off
setlocal enableextensions

set PROJ=%CD%
set SRC=%PROJ%\src
set DEBUG=-Od -Zi -Fd:shoki.pdb
set RELEASE=-O2
set TARGET=%DEBUG%
set RUN_AFTER_BUILD=1

if not exist %PROJ%\build (mkdir %PROJ%\build)
pushd %PROJ%\build

cl -nologo %TARGET% %SRC%\main.cpp -Fe:shoki.exe ^
   -link kernel32.lib gdi32.lib gdiplus.lib comdlg32.lib user32.lib

if %RUN_AFTER_BUILD%==1 shoki.exe

popd
