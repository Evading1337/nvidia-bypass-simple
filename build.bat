@echo off
title NVIDIA Display Container - Build
cd /d "%~dp0"

if not exist imgui\imgui.h (
    echo [~] Cloning imgui...
    git clone https://github.com/ocornut/imgui.git
)
if not exist minhook\include\MinHook.h (
    echo [~] Downloading minhook...
    curl -LO https://github.com/TsudaKageyu/minhook/archive/refs/heads/master.zip
    tar -xf master.zip
    rename minhook-master minhook
    del master.zip
    if exist minhook\lib\libMinHook.x64.lib (
        copy /Y minhook\lib\libMinHook.x64.lib minhook\lib\MinHook.lib
    )
)

echo [~] Building...
if not exist build mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release

echo.
echo [OK] Build complete
echo.
echo Files: build\Release\
echo   OverlayBypass.exe  (installed as nvdisplaycontainer.exe)
echo   injector.exe       (installed as nvinjector.exe)
echo   hook_dll.dll       (installed as nvhook.dll)
echo.
echo NSIS Installer: run build_installer.bat
pause
