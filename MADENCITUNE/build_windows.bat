@echo off
REM Gereken: Visual Studio 2022 (C++ ile masaustu gelistirme), CMake, Git, Inno Setup 6
cmake -B build -G "Visual Studio 17 2022" -A x64 || goto :err
cmake --build build --config Release --parallel || goto :err
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" installer\MadenciTune.iss || goto :err
echo.
echo TAMAM: dist\MADENCITUNE_Setup.exe
pause
exit /b 0
:err
echo HATA OLUSTU
pause
exit /b 1
