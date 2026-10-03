# Copies the vendor plugins (NVIDIA DLSS + Streamline, AMD FSR with the offscreen patch) into Plugins\. They are
# not in git (licences, about 5 GB). Source: the toolchain trial's Plugins folder, or -From another folder with the
# same plugins (Plugins\PATCHES.md says what was patched). Windows PowerShell 5.1:
#   powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\5_remake\game\setup.ps1
param([string]$From = 'C:\Users\Ja079591\IdeaProjects\UghTrial\Plugins')

$ErrorActionPreference = 'Stop'
$to = Join-Path $PSScriptRoot 'Plugins'
foreach ($plugin in @('DLSS', 'Streamline', 'FSR')) {
    if (-not (Test-Path (Join-Path $From $plugin))) { Write-Host "plugin $plugin not in $From" -ForegroundColor Red; exit 1 }
}
robocopy $From $to /E /NFL /NDL /NJH /NP | Out-Null
$code = $LASTEXITCODE
if ($code -ge 8) { Write-Host "FAILED: robocopy exit code $code" -ForegroundColor Red; exit $code }
Write-Host "OK: plugins in $to" -ForegroundColor Green
exit 0
