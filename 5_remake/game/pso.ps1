# Records the bundled PSO cache, so the packaged game does not stutter when it first draws something:
#   1. package.ps1 (the cook writes the stable shader keys, *.shk)
#   2. the packaged game shows the menu and plays a few levels by itself without a window (-UghShot,
#      -RenderOffscreen) with -logPSO
#   3. ShaderPipelineCacheTools expands the recorded PSOs with the keys into Build\Windows\PipelineCaches
#   4. package.ps1 again: the cache goes into the package (and the zip)
# Windows PowerShell 5.1:
#   powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\5_remake\game\pso.ps1
param([int]$TimeoutSeconds = 600)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'ue.ps1')
$package = Join-Path $PSScriptRoot 'package.ps1'
$game = Join-Path $PSScriptRoot 'Packaged\Windows'
$recorded = Join-Path $game 'UghGame\Saved\CollectedPSOs'
$keys = Join-Path $PSScriptRoot 'Saved\Cooked\Windows\UghGame\Metadata\PipelineCaches'
$caches = Join-Path $PSScriptRoot 'Build\Windows\PipelineCaches'
# the game runs on DX12 with shader model 6: the cache, the recording and the keys must all be of that format
$format = 'PCD3D_SM6'
$cache = Join-Path $caches "PSO_UghGame_$format.spc"

# an old cache would go into this cook too (and break it when the shaders changed)
if (Test-Path $cache) { Remove-Item -Force $cache }
& powershell -ExecutionPolicy Bypass -File $package -NoZip
if ($LASTEXITCODE -ne 0) { Write-Host 'FAILED: first package' -ForegroundColor Red; exit 1 }

if (Test-Path $recorded) { Remove-Item -Recurse -Force $recorded }
$shots = Join-Path $PSScriptRoot 'Saved\Shots\Pso'
# the menu, a calm and a windy level of one player, the team
$arguments = "-logPSO `"-UghShot=$shots`" -UghShotMenu -UghShotLevels=1p:1,1p:43,team:1 -UghShotAt=5"
$code = Invoke-UghOffscreen (Join-Path $game 'UghGame.exe') $arguments $TimeoutSeconds
if ($code -eq -1) {
    Write-Host "FAILED: the recording run did not end in $TimeoutSeconds s" -ForegroundColor Red
    exit 1
}
$records = @(Get-ChildItem $recorded -Filter '*.rec.upipelinecache' -ErrorAction SilentlyContinue)
$stableKeys = @(Get-ChildItem $keys -Filter "*-$format.shk" -ErrorAction SilentlyContinue)
if ($code -ne 0 -or $records.Count -eq 0 -or $stableKeys.Count -eq 0) {
    Write-Host "FAILED: exit code $code, $($records.Count) recordings, $($stableKeys.Count) key files" -ForegroundColor Red
    exit 1
}

New-Item -ItemType Directory -Force $caches | Out-Null
$log = Join-Path $PSScriptRoot 'Saved\Logs\UghPso.log'
$files = @($records | ForEach-Object { "`"$($_.FullName)`"" }) + @($stableKeys | ForEach-Object { "`"$($_.FullName)`"" })
$expand = "`"$Project`" -run=ShaderPipelineCacheTools expand $($files -join ' ') `"$cache`" -unattended -nopause `"-abslog=$log`""
$tool = Start-Process -FilePath $UeEditorCmd -ArgumentList $expand -Wait -PassThru -NoNewWindow
if ($tool.ExitCode -ne 0 -or -not (Test-Path $cache)) { Write-Host "FAILED: expand (exit code $($tool.ExitCode)), see $log" -ForegroundColor Red; exit 1 }
Write-Host "cache: $cache ($((Get-Item $cache).Length) bytes)"
# what the recording run saved next to the game (logs, the recording) does not belong into the package
Remove-Item -Recurse -Force (Join-Path $game 'UghGame\Saved')

& powershell -ExecutionPolicy Bypass -File $package
if ($LASTEXITCODE -ne 0) { Write-Host 'FAILED: second package' -ForegroundColor Red; exit 1 }
Write-Host 'OK' -ForegroundColor Green
exit 0
