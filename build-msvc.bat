@echo off
setlocal enableextensions

set SRC=%CD%\src

if not exist build (mkdir build)
pushd build

cl -Od %SRC%\main.cpp -Fe:pwThief.exe

popd
