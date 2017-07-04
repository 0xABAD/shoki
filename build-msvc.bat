@echo off
setlocal enableextensions

set PROJ=%CD%
set SRC=%PROJ%\src
set DEBUG=-Od -Zi -Fd:shoki.pdb -DDEBUG
set RELEASE=-O2
set TARGET=%RELEASE%
set RUN_AFTER_BUILD=0

if not exist %PROJ%\build (mkdir %PROJ%\build)
pushd %PROJ%\build

cl -nologo %TARGET% %SRC%\main.cpp -Fe:shoki.exe ^
   -link gdi32.lib gdiplus.lib user32.lib

if %RUN_AFTER_BUILD%==1 shoki.exe

popd
