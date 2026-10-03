# Builds the editor of the UE project (modules UghLogic and UghGame). Windows PowerShell 5.1:
#   powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\5_remake\game\build.ps1
# Needs the plugins (setup.ps1) and Unreal Engine 5.8 (ue.ps1).
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'ue.ps1')
if (-not (Test-Path (Join-Path $PSScriptRoot 'Plugins\DLSS'))) {
    Write-Host 'no plugins: run setup.ps1 first' -ForegroundColor Red; exit 1
}
& $UeBuild UghGameEditor Win64 Development "-Project=$Project" -WaitMutex -NoHotReloadFromIDE
$code = $LASTEXITCODE
if ($code -ne 0) { Write-Host "FAILED (exit code $code)" -ForegroundColor Red; exit $code }
Write-Host 'OK' -ForegroundColor Green
exit 0
