# Builds the editor of the UE project (modules UghLogic, UghGame, UghEditor), then makes the materials
# (commandlet UghMakeAssets -> Content\Generated). Windows PowerShell 5.1:
#   powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\5_remake\game\build.ps1
# Needs the plugins (setup.ps1) and Unreal Engine 5.8 (ue.ps1). -NoAssets only compiles.
param([switch]$NoAssets)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'ue.ps1')
if (-not (Test-Path (Join-Path $PSScriptRoot 'Plugins\DLSS'))) {
    Write-Host 'no plugins: run setup.ps1 first' -ForegroundColor Red; exit 1
}
& $UeBuild UghGameEditor Win64 Development "-Project=$Project" -WaitMutex -NoHotReloadFromIDE
$code = $LASTEXITCODE
if ($code -ne 0) { Write-Host "FAILED: build (exit code $code)" -ForegroundColor Red; exit $code }
if (-not $NoAssets) {
    $log = Join-Path $PSScriptRoot 'Saved\Logs\UghMakeAssets.log'
    & $UeEditorCmd $Project -run=UghMakeAssets -unattended -nopause -nosplash -nosound "-abslog=$log" | Out-Null
    $code = $LASTEXITCODE
    if ($code -ne 0) { Write-Host "FAILED: materials (exit code $code), see $log" -ForegroundColor Red; exit $code }
}
Write-Host 'OK' -ForegroundColor Green
exit 0
