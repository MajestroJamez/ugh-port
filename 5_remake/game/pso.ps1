# Records the bundled PSO cache, so the packaged game does not stutter when it first draws something:
#   1. package.ps1 (the cook writes the stable shader keys, *.shk)
#   2. the packaged game plays by itself without a window (-UghShot, -RenderOffscreen) with -logPSO, in a few runs
#      (Passes): the menu and its screens, the quick set of levels.ps1 (every mood, rising water, storms, the team),
#      the flight to the sea stack in each mood, the bursts of the events by day and at night, the card of a game's
#      end with a high score, the level selection over the archipelago (its flights, choosing; both modes), a copter
#      flying fast (its motion blur; at Medium too), the quick
#      set, the flights, the bursts and the level selection at the preset Low and a few levels at Medium
#      (their shaders differ: no volumetric fog, no halo, the sea without reflections)
#   3. ShaderPipelineCacheTools expands the recorded PSOs with the keys into Build\Windows\PipelineCaches
#   4. package.ps1 again: the cache goes into the package (and the zip)
# Windows PowerShell 5.1:
#   powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\5_remake\game\pso.ps1
param([int]$TimeoutSeconds = 1200)

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
$shots = Join-Path $PSScriptRoot 'Saved\Shots\Pso'
$low = Join-Path $shots 'profile-low.json'
$medium = Join-Path $shots 'profile-medium.json'
New-UghQualityProfile $low 'Low'
New-UghQualityProfile $medium 'Medium'
$Passes = @(
    "-UghShotMenu -UghShotScreens=settings,controls,scores -UghShotLevels=$UghQuickLevels -UghShotAt=3",
    '-UghShotLevels=1p:1,1p:3,1p:5,1p:6,1p:43 -UghShotIntro=4.4',
    '-UghShotLevels=1p:1,1p:6 -UghShotEffect=all',
    '-UghShotLevels=1p:3 -UghShotEnd -UghShotScore=5000',
    '-UghShotLevels=1p:1,team:1 -UghShotIsles=over:2,over:4,choose,approach:1,arrive',
    # a copter flying fast: its motion blur (FUghMotionBlur); at medium too (half-resolution blur)
    '-UghShotLevels=1p:1 -UghShotRush=left -UghShotRushY=60 -UghShotRushAfter=0.3,1.2',
    # low and medium (their shaders differ: no volumetric fog, no halo, the sea without reflections, no fire shadows):
    # every mood and the team at low (the notebook's preset), its flights and bursts too; a few levels at medium
    "-UghShotLevels=$UghQuickLevels -UghShotAt=2 `"-UghProfile=$low`"",
    "-UghShotLevels=1p:1,1p:3,1p:6,1p:23,1p:43 -UghShotIntro=4.4 `"-UghProfile=$low`"",
    "-UghShotLevels=1p:1,1p:6 -UghShotEffect=all `"-UghProfile=$low`"",
    "-UghShotLevels=1p:1 -UghShotIsles=over:2,over:4,choose,approach:1,arrive `"-UghProfile=$low`"",
    "-UghShotLevels=1p:1,1p:6,1p:43,team:21 -UghShotAt=2 `"-UghProfile=$medium`"",
    "-UghShotLevels=1p:1 -UghShotRush=left -UghShotRushY=60 -UghShotRushAfter=1.2 `"-UghProfile=$medium`""
)

# an old cache would go into this cook too (and break it when the shaders changed)
if (Test-Path $cache) { Remove-Item -Force $cache }
& powershell -ExecutionPolicy Bypass -File $package -NoZip
if ($LASTEXITCODE -ne 0) { Write-Host 'FAILED: first package' -ForegroundColor Red; exit 1 }

if (Test-Path $recorded) { Remove-Item -Recurse -Force $recorded }
$pass = 0
foreach ($arguments in $Passes) {
    $pass++
    $log = Join-Path $PSScriptRoot "Saved\Logs\UghPsoRun$pass.log"
    $code = Invoke-UghOffscreen (Join-Path $game 'UghGame.exe') "-logPSO `"-UghShot=$shots`" `"-abslog=$log`" $arguments" $TimeoutSeconds
    if ($code -ne 0) {
        Write-Host "FAILED: recording run $pass (exit code $code; -1: no end in $TimeoutSeconds s), see $log" -ForegroundColor Red
        exit 1
    }
    Write-Host "recorded run $pass of $($Passes.Count)"
}
$records = @(Get-ChildItem $recorded -Filter '*.rec.upipelinecache' -ErrorAction SilentlyContinue)
$stableKeys = @(Get-ChildItem $keys -Filter "*-$format.shk" -ErrorAction SilentlyContinue)
if ($records.Count -lt $Passes.Count -or $stableKeys.Count -eq 0) {
    Write-Host "FAILED: $($records.Count) recordings of $($Passes.Count) runs, $($stableKeys.Count) key files" -ForegroundColor Red
    exit 1
}

New-Item -ItemType Directory -Force $caches | Out-Null
$log = Join-Path $PSScriptRoot 'Saved\Logs\UghPso.log'
$files = @($records | ForEach-Object { "`"$($_.FullName)`"" }) + @($stableKeys | ForEach-Object { "`"$($_.FullName)`"" })
$expand = "`"$Project`" -run=ShaderPipelineCacheTools expand $($files -join ' ') `"$cache`" -unattended -nopause `"-abslog=$log`""
$tool = Start-Process -FilePath $UeEditorCmd -ArgumentList $expand -Wait -PassThru -NoNewWindow
if ($tool.ExitCode -ne 0 -or -not (Test-Path $cache)) { Write-Host "FAILED: expand (exit code $($tool.ExitCode)), see $log" -ForegroundColor Red; exit 1 }
$psos = Select-String -Path $log -Pattern 'Wrote (\d+) binary PSOs' | Select-Object -Last 1
Write-Host "cache: $cache ($((Get-Item $cache).Length) bytes) $(if ($psos) { $psos.Line.Trim() })"
# what the recording runs saved next to the game (logs, the recordings) does not belong into the package
Remove-Item -Recurse -Force (Join-Path $game 'UghGame\Saved')

& powershell -ExecutionPolicy Bypass -File $package
if ($LASTEXITCODE -ne 0) { Write-Host 'FAILED: second package' -ForegroundColor Red; exit 1 }
Write-Host 'OK' -ForegroundColor Green
exit 0
