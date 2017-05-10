@echo off
setlocal enableextensions

set SRC=%CD%\src
set DEBUG=-g
set RELEASE=-O3
set TARGET=%DEBUG%
set RUN_AFTER_BUILD=1

if not exist build (mkdir build)
pushd build

g++ %TARGET% %SRC%\main.cpp -o pwThief.exe -m64 -mwindows -mwin32

if %RUN_AFTER_BUILD%==1 pwThief.exe

popd
