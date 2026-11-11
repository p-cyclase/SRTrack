@echo off
setlocal
rc /nologo /c 65001 resource.rc || exit /b 1
cl /nologo /EHsc /utf-8 /std:c++17 /LD plugin.cpp resource.res /link /OUT:SRTrackExporter.aux2 comdlg32.lib user32.lib advapi32.lib || exit /b 1
cl /nologo /EHsc /utf-8 /std:c++17 test_core.cpp /Fe:test_core.exe || exit /b 1
test_core.exe || exit /b 1
cl /nologo /EHsc /utf-8 /std:c++17 test_inputs.cpp resource.res /Fe:test_inputs.exe /link comdlg32.lib user32.lib advapi32.lib || exit /b 1
test_inputs.exe || exit /b 1
echo Build and core tests passed.
