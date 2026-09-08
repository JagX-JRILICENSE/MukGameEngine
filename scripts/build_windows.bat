@echo off
setlocal enabledelayedexpansion

REM Muk Game Engine - Windows build workflow
REM Usage:
REM   scripts\build_windows.bat           -> minimal Release
REM   scripts\build_windows.bat full      -> ImGui + Jolt + tinygltf
REM   scripts\build_windows.bat debug     -> Debug minimal

set CONFIG=Release
set EXTRA=
set GEN=Visual Studio 17 2022

if /I "%1"=="full" (
  set EXTRA=-DMUK_USE_IMGUI=ON -DMUK_USE_JOLT=ON -DMUK_USE_TINYGLTF=ON
  echo [Muk] Full feature build: ImGui + Jolt + tinygltf
)
if /I "%1"=="debug" (
  set CONFIG=Debug
  echo [Muk] Debug build
)
if /I "%1"=="full-debug" (
  set CONFIG=Debug
  set EXTRA=-DMUK_USE_IMGUI=ON -DMUK_USE_JOLT=ON -DMUK_USE_TINYGLTF=ON
  echo [Muk] Full Debug build
)

cd /d "%~dp0.."
if not exist build mkdir build
cd build

echo [Muk] Configuring...
cmake .. -G "%GEN%" -A x64 %EXTRA%
if errorlevel 1 (
  echo [Muk] CMake configure FAILED
  exit /b 1
)

echo [Muk] Building %CONFIG%...
cmake --build . --config %CONFIG% -j %NUMBER_OF_PROCESSORS%
if errorlevel 1 (
  echo [Muk] Build FAILED
  exit /b 1
)

echo.
echo [Muk] Success. Binaries:
echo   build\bin\%CONFIG%\MukRuntime.exe
echo   build\bin\%CONFIG%\MukEditor.exe
echo.
endlocal
