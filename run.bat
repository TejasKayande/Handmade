
@echo off
setlocal

cd /D "%~dp0"

call "bin\win_main.exe"

popd
endlocal
