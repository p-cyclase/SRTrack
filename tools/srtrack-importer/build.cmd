@echo off
setlocal
cl /nologo /EHsc /utf-8 /std:c++17 /LD importer.cpp /link /OUT:SRTrackImporter.aux2 || exit /b 1
echo SRTrack Importer build passed.
