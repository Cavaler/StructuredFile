@echo off
for /f "usebackq tokens=3" %%V in (`grep PLUGIN_VERSION_MAJOR version.h`) do set V1=%%V
for /f "usebackq tokens=3" %%V in (`grep PLUGIN_VERSION_MINOR version.h`) do set V2=%%V
for /f "usebackq tokens=3" %%V in (`grep PLUGIN_VERSION_REVISION version.h`) do set V3=%%V

set VERSION=%V1%.%V2%%V3%
echo %VERSION%

ren StructuredFile*.7z StructuredFile%V1%%V2%%V3%.7z
"C:\Program Files\7-Zip\7z.exe" u -ur0 StructuredFile%V1%%V2%%V3%.7z
