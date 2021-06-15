@echo off
cd /d %~dp0
echo off

"../../../build/bin/MetadataGenerator" %1
