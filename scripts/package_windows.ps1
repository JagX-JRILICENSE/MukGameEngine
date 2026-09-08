param(
    [string]$Config = "Release",
    [string]$OutDir = "MukGameEngine-Windows-x64"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $Root

if (-not (Test-Path "build")) {
    Write-Host "No build/ folder. Run scripts/build_windows.ps1 -Full first." -ForegroundColor Red
    exit 1
}

if (Test-Path $OutDir) { Remove-Item -Recurse -Force $OutDir }
New-Item -ItemType Directory -Force -Path "$OutDir/bin" | Out-Null
New-Item -ItemType Directory -Force -Path "$OutDir/config" | Out-Null

$bin = "build/bin/$Config"
if (-not (Test-Path $bin)) { $bin = "build/bin" }

Copy-Item "$bin/MukRuntime.exe" "$OutDir/bin/" -ErrorAction Stop
Copy-Item "$bin/MukEditor.exe" "$OutDir/bin/" -ErrorAction Stop
Get-ChildItem $bin -Filter "*.dll" -ErrorAction SilentlyContinue | Copy-Item -Destination "$OutDir/bin/"

Copy-Item "README.md" "$OutDir/"
Copy-Item "LICENSE" "$OutDir/" -ErrorAction SilentlyContinue
Copy-Item "config/settings.example.ini" "$OutDir/config/"

@"
Muk Game Engine — install
=========================

1. Run bin\MukEditor.exe for realtime editor preview
2. Copy config\settings.example.ini to %APPDATA%\MukGameEngine\settings.ini
3. Paste your OpenRouter / NVIDIA / OpenAI / custom API key
4. Use the AI Control panel in the editor

This build is an early foundation, not full Unreal Engine feature parity.
"@ | Set-Content "$OutDir/INSTALL.txt"

$zip = "$OutDir.zip"
if (Test-Path $zip) { Remove-Item $zip }
Compress-Archive -Path $OutDir -DestinationPath $zip
Write-Host "Packaged: $zip" -ForegroundColor Green
