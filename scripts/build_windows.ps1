# Muk Game Engine - PowerShell build workflow
# Usage:
#   ./scripts/build_windows.ps1
#   ./scripts/build_windows.ps1 -Full
#   ./scripts/build_windows.ps1 -Config Debug -Full

param(
    [switch]$Full,
    [ValidateSet("Release", "Debug")]
    [string]$Config = "Release",
    [string]$Generator = "Visual Studio 17 2022"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $Root

$Extra = @()
if ($Full) {
    $Extra += "-DMUK_USE_IMGUI=ON", "-DMUK_USE_JOLT=ON", "-DMUK_USE_TINYGLTF=ON"
    Write-Host "[Muk] Full features: ImGui + Jolt + tinygltf" -ForegroundColor Cyan
}

if (-not (Test-Path build)) { New-Item -ItemType Directory -Path build | Out-Null }
Set-Location build

Write-Host "[Muk] Configuring..." -ForegroundColor Cyan
& cmake .. -G $Generator -A x64 @Extra
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }

Write-Host "[Muk] Building $Config..." -ForegroundColor Cyan
& cmake --build . --config $Config --parallel
if ($LASTEXITCODE -ne 0) { throw "Build failed" }

Write-Host ""
Write-Host "[Muk] Success" -ForegroundColor Green
Write-Host "  build/bin/$Config/MukRuntime.exe"
Write-Host "  build/bin/$Config/MukEditor.exe"
