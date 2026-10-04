# Copies the scanned assets of Epic's Electric Dreams sample the game uses (UghElectricDreams.h) with everything they
# need into Content\External\ElectricDreams (not in git: licensed for Unreal Engine projects, not ours to share),
# by the commandlet UghCopyElectricDreams; the sample itself is only read. Get the sample ("Electric Dreams
# Environment", Fab / Epic Games Launcher, for UE 5.8) first. Windows PowerShell 5.1:
#   powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\5_remake\game\electric-dreams.ps1
# -Source <folder> the sample's project folder (default: ..\Unreal Projects\ElectricDreamsEnv next to the repository).
# Run build.ps1 before (the commandlet is part of the editor). Without the copy the game shows the free assets.
param([string]$Source = '')

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'ue.ps1')
if (-not $Source) {
    $Source = Join-Path $PSScriptRoot '..\..\..\Unreal Projects\ElectricDreamsEnv'
}
$Source = [System.IO.Path]::GetFullPath($Source)
if (-not (Test-Path (Join-Path $Source 'ElectricDreamsEnv.uproject'))) {
    Write-Host "no Electric Dreams sample in $Source (-Source <folder>)" -ForegroundColor Red; exit 1
}
$log = Join-Path $PSScriptRoot 'Saved\Logs\UghCopyElectricDreams.log'
& $UeEditorCmd $Project -run=UghCopyElectricDreams "-Source=$Source" -unattended -nopause -nosplash -nosound "-abslog=$log" | Out-Null
$code = $LASTEXITCODE
if ($code -ne 0) { Write-Host "FAILED: copy (exit code $code), see $log" -ForegroundColor Red; exit $code }
$summary = Select-String -Path $log -Pattern 'LogUghCopyElectricDreams: Display: \d+ packages' | Select-Object -Last 1
if ($summary) { Write-Host ($summary.Line -replace '^.*Display: ', 'Electric Dreams: ') }
Write-Host 'OK' -ForegroundColor Green
exit 0
