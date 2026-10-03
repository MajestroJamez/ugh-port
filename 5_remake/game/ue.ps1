# Where Unreal Engine 5.8 is, for the scripts of this folder (dot-sourced). Another place: $env:UE_ROOT.
$UeRoot = if ($env:UE_ROOT) { $env:UE_ROOT } else { 'C:\Program Files\Epic Games\UE_5.8' }
$UeEditor = Join-Path $UeRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$UeEditorCmd = Join-Path $UeRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$UeBuild = Join-Path $UeRoot 'Engine\Build\BatchFiles\Build.bat'
$Project = Join-Path $PSScriptRoot 'UghGame.uproject'
if (-not (Test-Path $UeEditor)) { Write-Host "Unreal Engine not found in $UeRoot (set UE_ROOT)" -ForegroundColor Red; exit 1 }
