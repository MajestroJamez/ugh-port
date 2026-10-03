# Builds the game logic with VS Build Tools 2026 (MSVC, CMake and Ninja of the Build Tools) and runs its tests and
# the golden replays (CTest). Windows PowerShell 5.1:
#   powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\logic\build.ps1
# Needs assets\logic\ugh-data.ugd (.\gradlew.bat :extractor:run) and verify\build\replays (.\gradlew.bat :verify:replays).
# -NoTest only builds.
param([switch]$NoTest)

$ErrorActionPreference = 'Stop'
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) { Write-Host "vswhere not found: Visual Studio Build Tools missing" -ForegroundColor Red; exit 1 }
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { Write-Host "no Visual Studio with the C++ tools" -ForegroundColor Red; exit 1 }
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'

$src = $PSScriptRoot
$build = Join-Path $src 'build'
$steps = "cmake -S `"$src`" -B `"$build`" -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build `"$build`""
if (-not $NoTest) { $steps += " && ctest --test-dir `"$build`" -j 8 --output-on-failure" }
cmd /c "call `"$vcvars`" >nul && $steps"
$code = $LASTEXITCODE
if ($code -ne 0) { Write-Host "FAILED (exit code $code)" -ForegroundColor Red; exit $code }
Write-Host "OK" -ForegroundColor Green
