@echo off
cd /d %~dp0
echo off
cd ../../../build
CALL "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
ninja
