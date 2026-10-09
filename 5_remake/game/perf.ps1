# Measures the frame rate of the packaged game (package.ps1) at each quality preset without a window: for each preset
# a profile with that quality and the quick set of levels.ps1 (12 levels of every mood) from the package at a
# resolution, the frame rate of each level while its copters hover (FUghShot). At the end a table of the presets:
# the median, the slowest and the fastest level. Windows PowerShell 5.1:
#   powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\5_remake\game\perf.ps1
# -Presets: some of Low, Medium, High, Epic (separated by commas); -Resolution <width>x<height>; -At: seconds of each
# level measured (longer: the copters of a storm level may crash first); -PackageDir another package (an older one kept
# for an A/B), -Tag <name> its own shots and table, -Commands more console commands. The shots go to
# Saved\Shots\Levels-perf-<preset>[-<tag>], the table to Saved\Shots\perf[-<tag>].txt.
param([string]$Presets = 'Low,Medium,High,Epic', [string]$Resolution = '1920x1080', [double]$At = 2,
    [string]$PackageDir = 'Packaged\Windows', [string]$Tag = '', [string]$Commands = '')

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'ue.ps1')
$levels = Join-Path $PSScriptRoot 'levels.ps1'
$folder = Join-Path $PSScriptRoot 'Saved\Shots'
if ($Resolution -notmatch '^\d+x\d+$') { Write-Host '-Resolution wants <width>x<height>' -ForegroundColor Red; exit 1 }

$rows = @("fps of the package at $Resolution, $At s a level (the quick set: median, slowest, fastest)")
foreach ($preset in $Presets.Split(',')) {
    $run = 'perf-' + $preset.ToLower()
    if ($Tag) { $run += "-$Tag" }
    $exec = "r.SetRes ${Resolution}w"
    if ($Commands) { $exec += ",$Commands" }
    $presetProfile = Join-Path $folder "profile-$run.json"
    try { New-UghQualityProfile $presetProfile $preset }
    catch { Write-Host "FAILED: $_" -ForegroundColor Red; exit 1 }
    & powershell -ExecutionPolicy Bypass -File $levels -Quick -Package -At $At -Profile $presetProfile -Commands $exec -Tag $run -PackageDir $PackageDir
    if ($LASTEXITCODE -ne 0) { Write-Host "FAILED: levels.ps1 at $preset" -ForegroundColor Red; exit 1 }
    $fps = Join-Path $folder "Levels-$run\fps.txt"
    if (-not (Test-Path $fps)) { Write-Host "FAILED: no frame rates at $preset" -ForegroundColor Red; exit 1 }
    $rows += '{0,-7} {1}' -f $preset, [IO.File]::ReadAllText($fps)
}
$table = Join-Path $folder $(if ($Tag) { "perf-$Tag.txt" } else { 'perf.txt' })
[IO.File]::WriteAllLines($table, $rows)
$rows | ForEach-Object { Write-Host $_ }
Write-Host "OK: $table" -ForegroundColor Green
exit 0
