@echo off
setlocal

where wsl.exe >nul 2>&1
if errorlevel 1 (
    echo WSL is required to run this pipeline from Windows.
    echo Install WSL with a Linux distribution, then run this script again.
    exit /b 1
)

echo Starting the pipeline in WSL...
wsl.exe --cd "%~dp0" bash ./run_all.sh
set "RESULT=%ERRORLEVEL%"

if not "%RESULT%"=="0" (
    echo Pipeline failed with exit code %RESULT%.
)

exit /b %RESULT%
