@echo off
title Raavox Installer Builder
cd /d "%~dp0"

echo ============================================
echo   Raavox NVIDIA Bypass - Installer Builder
echo ============================================
echo.

:: Prüfen ob NSIS existiert
set NSIS_PATH=C:\Program Files (x86)\NSIS
if not exist "%NSIS_PATH%\makensis.exe" (
    echo [!!] NSIS not found at %NSIS_PATH%
    echo.
    echo Download: https://nsis.sourceforge.io/Download
    echo Install, then run this again.
    echo.
    pause
    exit /b 1
)

:: Build the projects first
echo [~] Building projects...
if not exist "build\Release\OverlayBypass.exe" (
    echo [!] Build not found. Running cmake...
    if not exist "imgui\imgui.h" (
        git clone https://github.com/ocornut/imgui.git
    )
    if not exist "build" mkdir build
    cd build
    cmake .. -G "Visual Studio 17 2022" -A x64
    cmake --build . --config Release
    cd ..
)

:: Compile NSIS installer
echo.
echo [~] Compiling installer...
"%NSIS_PATH%\makensis.exe" installer.nsi

if exist "RaavoxBypass_Setup.exe" (
    echo.
    echo [OK] Installer created: RaavoxBypass_Setup.exe
) else (
    echo.
    echo [!!] Installer creation failed
)

echo.
pause
