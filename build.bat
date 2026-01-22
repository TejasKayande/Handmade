@echo off
setlocal

cd /D "%~dp0"

if not exist "bin" (mkdir bin)

pushd bin

call cl /std:c++17 /FC /EHsc /MP /Zi /W3 ../win_main.cpp User32.lib Gdi32.lib 

popd
endlocal
