@echo off
setlocal enableextensions

set PROJ=%CD%
set SRC=%PROJ%\src
set DEBUG=-g
set RELEASE=-O3
set TARGET=%DEBUG%
set RUN_AFTER_BUILD=1

if not exist %PROJ%\build (mkdir %PROJ%\build)
pushd %PROJ%\build

g++ %TARGET% %SRC%\main.cpp -o shoki.exe -m64 -mwindows -mwin32 -lgdiplus

if %RUN_AFTER_BUILD%==1 shoki.exe

popd
