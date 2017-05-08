@echo off
setlocal enableextensions

set SRC=%CD%\src
set DEBUG=-Od -Zi -Fd:pwThief.pdb
set RELEASE=-O2
set TARGET=%DEBUG%

if not exist build (mkdir build)
pushd build

cl -nologo %TARGET% %SRC%\main.cpp -Fe:pwThief.exe ^
   -link kernel32.lib gdi32.lib comdlg32.lib user32.lib

popd
