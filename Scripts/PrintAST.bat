@echo off
cd /d %~dp0
echo off

"../../../build/bin/clang-check" -ast-dump %1
