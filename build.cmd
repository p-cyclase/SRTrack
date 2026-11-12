@echo off
setlocal

set "OUT=out"
set "PLUGIN_DIR=%OUT%\Plugin\SRTrack"
set "SCRIPT_DIR=%OUT%\Script\SRTrack"
set "OBJ_DIR=%OUT%\obj"

if not exist "%PLUGIN_DIR%" mkdir "%PLUGIN_DIR%" || exit /b 1
if not exist "%SCRIPT_DIR%" mkdir "%SCRIPT_DIR%" || exit /b 1
if not exist "%OBJ_DIR%" mkdir "%OBJ_DIR%" || exit /b 1

rc /nologo /c 65001 /fo "%OUT%\resource.res" src\aux2\resource.rc || exit /b 1
cl /nologo /EHsc /utf-8 /std:c++17 /LD /Fo"%OBJ_DIR%\\" src\aux2\exporter.cpp src\aux2\importer.cpp src\aux2\monitor.cpp "%OUT%\resource.res" /link /OUT:"%PLUGIN_DIR%\SRTrack.aux2" /IMPLIB:"%OBJ_DIR%\SRTrack.lib" comdlg32.lib user32.lib advapi32.lib || exit /b 1
cl /nologo /EHsc /utf-8 /std:c++17 /LD /Fo"%OBJ_DIR%\\" src\mod2\module.cpp /link /OUT:"%SCRIPT_DIR%\SRTrack.mod2" /IMPLIB:"%OBJ_DIR%\SRTrackModule.lib" || exit /b 1
copy /y src\lua\SRTrack.lua "%PLUGIN_DIR%\SRTrack.lua" >nul || exit /b 1
copy /y src\object\SRTrack.obj2 "%SCRIPT_DIR%\SRTrack.obj2" >nul || exit /b 1

cl /nologo /EHsc /utf-8 /std:c++17 /Fo"%OBJ_DIR%\\" tests\test_core.cpp /Fe:"%OUT%\test_core.exe" || exit /b 1
"%OUT%\test_core.exe" || exit /b 1
cl /nologo /EHsc /utf-8 /std:c++17 /Fo"%OBJ_DIR%\\" tests\test_inputs.cpp src\aux2\importer.cpp src\aux2\monitor.cpp "%OUT%\resource.res" /Fe:"%OUT%\test_inputs.exe" /link comdlg32.lib user32.lib advapi32.lib || exit /b 1
"%OUT%\test_inputs.exe" || exit /b 1

echo Build and tests passed.
