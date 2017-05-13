@echo off
setlocal enableextensions

set SRC=%CD%\src
set DEBUG=-Od -Zi -Fd:pwThief.pdb
set RELEASE=-O2
set TARGET=%DEBUG%
set RUN_AFTER_BUILD=1

if not exist build (mkdir build)
pushd build

cl -nologo %TARGET% %SRC%\main.cpp -Fe:pwThief.exe ^
   -link kernel32.lib gdi32.lib gdiplus.lib comdlg32.lib user32.lib

if %RUN_AFTER_BUILD%==1 pwThief.exe

popd
