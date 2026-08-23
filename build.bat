@echo off
setlocal

cd /D "%~dp0"

if not exist "bin" (mkdir bin)

pushd bin

call cl /FC /EHsc /MP /Zi /W2 ../win_main.cpp User32.lib Gdi32.lib 

popd
endlocal
