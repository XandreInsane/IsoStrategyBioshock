@echo off
setlocal
rem If MSYS2 is installed outside C:\msys64, update the path below.
set "PATH=C:\msys64\ucrt64\bin;%PATH%"
cd /d "%~dp0"
if not exist "%~dp0build\rapture.exe" (
    echo Rapture has not been built yet. Configuring the project first...
    cmake -S "%~dp0" -B "%~dp0build" -G Ninja -DCMAKE_BUILD_TYPE=Release
    if errorlevel 1 goto build_failed
)
rem Rebuild whenever source files changed; do not silently launch an outdated exe.
cmake --build "%~dp0build"
if errorlevel 1 goto build_failed
start "Rapture: Dream of the Deep" "%~dp0build\rapture.exe"
goto done
:build_failed
echo Build failed. Check raylib/MSYS2 setup and see README_FR.md.
pause
exit /b 1
:done
endlocal
