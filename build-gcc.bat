@echo off
setlocal enableextensions

set SRC=%CD%\src
set DEBUG=-g
set TARGET=%DEBUG%

if not exist build (mkdir build)
pushd build

g++ %TARGET% %SRC%\main.cpp -o pwThief.exe -m64 -mwindows -mwin32

pwThief.exe

popd
