@echo off
rem Run from the project root in a Visual Studio developer command prompt (Windows SDK required).
cl /nologo /std:c++17 /EHsc /Itests\host /Iinclude tests\config_tests.cpp /Fobuild-ultra\config_tests.obj /Febuild-ultra\config_tests.exe
if errorlevel 1 exit /b 1
build-ultra\config_tests.exe
