# Builds the editor of the UE project (modules UghLogic, UghGame, UghEditor), makes the materials (commandlet
# UghMakeAssets -> Content\Generated), bakes the flames (commandlet UghMakeFlames: a fire simulated into flipbooks ->
# Content\Generated, a look at them in Saved\Flames; about 2 minutes) and imports the 3D assets fetch-assets.ps1
# downloaded (commandlet UghImportAssets -> Content\Imported; an asset not downloaded is skipped, the game shows clay
# shapes instead). Windows PowerShell 5.1:
#   powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\5_remake\game\build.ps1
# Needs the plugins (setup.ps1) and Unreal Engine 5.8 (ue.ps1). -NoAssets only compiles; -ForceImport imports every
# downloaded asset again.
param([switch]$NoAssets, [switch]$ForceImport)

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
    $log = Join-Path $PSScriptRoot 'Saved\Logs\UghMakeFlames.log'
    & $UeEditorCmd $Project -run=UghMakeFlames -unattended -nopause -nosplash -nosound "-abslog=$log" | Out-Null
    $code = $LASTEXITCODE
    if ($code -ne 0) { Write-Host "FAILED: flames (exit code $code), see $log" -ForegroundColor Red; exit $code }
    $log = Join-Path $PSScriptRoot 'Saved\Logs\UghImportAssets.log'
    $arguments = @($Project, '-run=UghImportAssets', '-unattended', '-nopause', '-nosplash', '-nosound', "-abslog=$log")
    if ($ForceImport) { $arguments += '-Force' }
    & $UeEditorCmd @arguments | Out-Null
    $code = $LASTEXITCODE
    if ($code -ne 0) { Write-Host "FAILED: 3D assets (exit code $code), see $log" -ForegroundColor Red; exit $code }
    $summary = Select-String -Path $log -Pattern 'LogUghImportAssets: Display: \d+ imported' | Select-Object -Last 1
    if ($summary) { Write-Host ($summary.Line -replace '^.*Display: ', '3D assets: ') }
}
Write-Host 'OK' -ForegroundColor Green
exit 0
