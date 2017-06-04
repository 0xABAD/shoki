@echo off
setlocal enableextensions

set SRC=%CD%\src
set DEBUG=-g
set RELEASE=-O3
set TARGET=%DEBUG%
set RUN_AFTER_BUILD=1

if not exist build (mkdir build)
pushd build

g++ %TARGET% %SRC%\main.cpp -o shoki.exe -m64 -mwindows -mwin32 -lgdiplus

if %RUN_AFTER_BUILD%==1 shoki.exe

popd
