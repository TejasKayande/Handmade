@echo off
setlocal

cd /D "%~dp0"

if not exist "bin" (mkdir bin)

pushd bin

call cl /MT /nologo /GR- /WX /W3 /DHANDMADE_SLOW=1 /DHANDMADE_INTERNAL=1 /FC /EHsc /MP /Z7 /Fmwin32_handmade.map ../code/win_main.cpp User32.lib Gdi32.lib 

popd
endlocal
