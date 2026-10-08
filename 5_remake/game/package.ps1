# Packages the game for Windows into Packaged\Windows (Development: its log and the autopilot's lines that levels.ps1,
# perf.ps1 and pso.ps1 read, the console), with the data of assets\ next to it (Packaged\Windows\UghGame\assets: for
# your own use, the data is not ours to share). Without -NoZip also the game to play elsewhere: the same cook staged
# again as Shipping into Packaged\Shipping\Windows (no log, no console, no debug overlays of DLSS and Streamline; its
# profile in %LOCALAPPDATA%\UghGame\Saved) and zipped without .pdb into Packaged\UghGame-Windows.zip. A PSO cache in
# Build\Windows\PipelineCaches (pso.ps1) goes into both. Windows PowerShell 5.1:
#   powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\5_remake\game\package.ps1
param([switch]$NoZip)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'ue.ps1')
$uat = Join-Path $UeRoot 'Engine\Build\BatchFiles\RunUAT.bat'
$out = Join-Path $PSScriptRoot 'Packaged'
$assets = Join-Path $PSScriptRoot '..\..\assets'
if (-not (Test-Path (Join-Path $assets 'logic\ugh-data.ugd'))) { Write-Host 'no game data: .\gradlew.bat :extractor:run' -ForegroundColor Red; exit 1 }

# the materials are made by build.ps1
& powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'build.ps1')
if ($LASTEXITCODE -ne 0) { Write-Host 'FAILED: build' -ForegroundColor Red; exit 1 }

# UAT talks to the Zen server on [::1]; without ::1 in NO_PROXY .NET sends that through the HTTP proxy
$noProxy = @($env:NO_PROXY, '::1', '[::1]') | Where-Object { $_ }
$env:NO_PROXY = $noProxy -join ','

# BuildCookRun of configuration $Config archived into $Archive\Windows (a fresh one: nothing of an older package - an old
# pak, its assets - stays next to the new one), its log in Saved\Logs\$LogName; then the data of assets\ next to it
function Invoke-UghPackage([string]$Config, [string]$Archive, [string]$Steps, [string]$LogName) {
    $log = Join-Path $PSScriptRoot "Saved\Logs\$LogName"
    $windows = Join-Path $Archive 'Windows'
    if (Test-Path $windows) { Remove-Item -Recurse -Force $windows }
    # through cmd, so the log stays UTF-8 and PowerShell does not turn UAT's stderr into errors
    cmd /c "`"$uat`" BuildCookRun `"-project=$Project`" -platform=Win64 -clientconfig=$Config $Steps -stage -pak -archive `"-archivedirectory=$Archive`" -unattended -utf8output -nop4 > `"$log`" 2>&1"
    if ($LASTEXITCODE -ne 0) { Write-Host "FAILED: BuildCookRun $Config (exit code $LASTEXITCODE), see $log" -ForegroundColor Red; return $false }

    $data = Join-Path $windows 'UghGame\assets'
    if (Test-Path $data) { Remove-Item -Recurse -Force $data }
    New-Item -ItemType Directory -Force (Join-Path $data 'logic') | Out-Null
    Copy-Item (Join-Path $assets 'logic\ugh-data.ugd') (Join-Path $data 'logic')
    Copy-Item (Join-Path $assets 'sprites.json'), (Join-Path $assets 'levels.json') $data
    Copy-Item -Recurse (Join-Path $assets 'sprites') $data
    # the sounds (.\gradlew.bat :extractor:sound); without them the game is silent
    if (Test-Path (Join-Path $assets 'sound')) { Copy-Item -Recurse (Join-Path $assets 'sound') $data }
    else { Write-Host 'no sounds (.\gradlew.bat :extractor:sound): the package is silent' -ForegroundColor Yellow }
    # the fonts of the menu and the HUD with their licenses (fetch-assets.ps1); without them the engine's Roboto
    $fonts = Join-Path $assets '3d\googlefonts'
    if (Test-Path $fonts) {
        New-Item -ItemType Directory -Force (Join-Path $data '3d') | Out-Null
        Copy-Item -Recurse $fonts (Join-Path $data '3d')
    }
    else { Write-Host 'no fonts (fetch-assets.ps1): the package shows the engine''s Roboto' -ForegroundColor Yellow }
    $size = (Get-ChildItem -Recurse -File $windows | Where-Object { $_.Extension -ne '.pdb' } | Measure-Object -Sum Length).Sum
    Write-Host ('{0}: {1} ({2:N2} GB without .pdb)' -f $Config, $windows, ($size / 1GB))
    return $true
}

if (-not (Invoke-UghPackage 'Development' $out '-build -cook' 'UghPackage.log')) { exit 1 }
if (-not $NoZip) {
    # the same cook (in the Zen store of Saved\Cooked) staged again with the Shipping executable
    $shipping = Join-Path $out 'Shipping'
    if (-not (Invoke-UghPackage 'Shipping' $shipping "-build -skipcook `"-stagingdirectory=$(Join-Path $PSScriptRoot 'Saved\StagedShipping')`"" 'UghPackageShipping.log')) { exit 1 }

    $zip = Join-Path $out 'UghGame-Windows.zip'
    if (Test-Path $zip) { Remove-Item -Force $zip }
    $staging = Join-Path $out 'zip'
    if (Test-Path $staging) { Remove-Item -Recurse -Force $staging }
    # without the .pdb files and what a run of the packaged game saved next to itself
    robocopy (Join-Path $shipping 'Windows') $staging /E /XF *.pdb /XD Saved /NFL /NDL /NJH /NJS /NP | Out-Null
    if ($LASTEXITCODE -ge 8) { Write-Host "FAILED: copy for the zip (robocopy $LASTEXITCODE)" -ForegroundColor Red; exit 1 }
    # .NET's zip (Zip64: the paks can be bigger than the 2 GB Compress-Archive can take); they are compressed already
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    [System.IO.Compression.ZipFile]::CreateFromDirectory($staging, $zip, [System.IO.Compression.CompressionLevel]::Fastest, $false)
    Remove-Item -Recurse -Force $staging
    Write-Host ('zip: {0} ({1:N2} GB)' -f $zip, ((Get-Item $zip).Length / 1GB))
}
Write-Host "OK: $(Join-Path $out 'Windows\UghGame.exe')" -ForegroundColor Green
exit 0
