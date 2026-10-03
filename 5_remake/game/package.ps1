# Packages the game for Windows (Development) into Packaged\Windows, with the data of assets\ next to it
# (Packaged\Windows\UghGame\assets: for your own use, the data is not ours to share), and a zip without .pdb files.
# A PSO cache in Build\Windows\PipelineCaches (pso.ps1) goes into the package. Windows PowerShell 5.1:
#   powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\5_remake\game\package.ps1
param([switch]$NoZip)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'ue.ps1')
$uat = Join-Path $UeRoot 'Engine\Build\BatchFiles\RunUAT.bat'
$out = Join-Path $PSScriptRoot 'Packaged'
$assets = Join-Path $PSScriptRoot '..\..\assets'
$log = Join-Path $PSScriptRoot 'Saved\Logs\UghPackage.log'
if (-not (Test-Path (Join-Path $assets 'logic\ugh-data.ugd'))) { Write-Host 'no game data: .\gradlew.bat :extractor:run' -ForegroundColor Red; exit 1 }

# the materials are made by build.ps1
& powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'build.ps1')
if ($LASTEXITCODE -ne 0) { Write-Host 'FAILED: build' -ForegroundColor Red; exit 1 }

# UAT talks to the Zen server on [::1]; without ::1 in NO_PROXY .NET sends that through the HTTP proxy
$noProxy = @($env:NO_PROXY, '::1', '[::1]') | Where-Object { $_ }
$env:NO_PROXY = $noProxy -join ','
# through cmd, so the log stays UTF-8 and PowerShell does not turn UAT's stderr into errors
cmd /c "`"$uat`" BuildCookRun `"-project=$Project`" -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive `"-archivedirectory=$out`" -unattended -utf8output -nop4 > `"$log`" 2>&1"
$code = $LASTEXITCODE
if ($code -ne 0) { Write-Host "FAILED: BuildCookRun (exit code $code), see $log" -ForegroundColor Red; exit $code }

$game = Join-Path $out 'Windows\UghGame'
$data = Join-Path $game 'assets'
if (Test-Path $data) { Remove-Item -Recurse -Force $data }
New-Item -ItemType Directory -Force (Join-Path $data 'logic') | Out-Null
Copy-Item (Join-Path $assets 'logic\ugh-data.ugd') (Join-Path $data 'logic')
Copy-Item (Join-Path $assets 'sprites.json'), (Join-Path $assets 'levels.json') $data
Copy-Item -Recurse (Join-Path $assets 'sprites') $data

if (-not $NoZip) {
    $zip = Join-Path $out 'UghGame-Windows.zip'
    if (Test-Path $zip) { Remove-Item -Force $zip }
    $staging = Join-Path $out 'zip'
    if (Test-Path $staging) { Remove-Item -Recurse -Force $staging }
    # without the .pdb files and what a run of the packaged game saved next to itself (pso.ps1)
    robocopy (Join-Path $out 'Windows') $staging /E /XF *.pdb /XD Saved /NFL /NDL /NJH /NJS /NP | Out-Null
    if ($LASTEXITCODE -ge 8) { Write-Host "FAILED: copy for the zip (robocopy $LASTEXITCODE)" -ForegroundColor Red; exit 1 }
    Compress-Archive -Path (Join-Path $staging '*') -DestinationPath $zip
    Remove-Item -Recurse -Force $staging
    Write-Host "zip: $zip"
}
Write-Host "OK: $(Join-Path $out 'Windows\UghGame.exe')" -ForegroundColor Green
exit 0
