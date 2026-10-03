# Takes a screenshot of every level of both modes without a window and puts them on contact sheets. One game process:
# the autopilot of FUghShot starts each level from the menu as a player would (the mode, the level's password, Enter),
# lets the copters hover, takes the screenshot and gives the game up (Esc), back to the menu. The screenshots go to
# Saved\Shots\Levels\<mode>-<NN>.png (and the menu's menu.png), the sheets to Saved\Shots\Levels\levels-<mode>.png.
# Windows PowerShell 5.1:
#   powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\5_remake\game\levels.ps1
# -Levels: other levels, items <1p|team>:<first>[-<last>]; -At: seconds of the fully shown level before each shot.
param([string]$Levels = '1p:1-69,team:1-81', [double]$At = 1.5, [int]$TimeoutSeconds = 3600)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'ue.ps1')
Add-Type -AssemblyName System.Drawing
$folder = Join-Path $PSScriptRoot 'Saved\Shots\Levels'
$log = Join-Path $PSScriptRoot 'Saved\Logs\UghLevels.log'

# the screenshots the list asks for, by mode
$expected = [ordered]@{}
foreach ($item in $Levels.Split(',')) {
    if ($item -notmatch '^(1p|team):(\d+)(?:-(\d+))?$') {
        Write-Host "FAILED: $item is not <1p|team>:<first>[-<last>]" -ForegroundColor Red
        exit 1
    }
    $mode = $Matches[1]
    $first = [int]$Matches[2]
    $last = if ($Matches[3]) { [int]$Matches[3] } else { $first }
    if ($first -lt 1 -or $last -lt $first) {
        Write-Host "FAILED: $item is not a range of levels from 1" -ForegroundColor Red
        exit 1
    }
    if (-not $expected.Contains($mode)) { $expected[$mode] = New-Object System.Collections.ArrayList }
    for ($level = $first; $level -le $last; $level++) { $null = $expected[$mode].Add($level) }
}

if (Test-Path $folder) { Remove-Item -Recurse -Force $folder }
$null = New-Item -ItemType Directory -Force $folder
$started = Get-Date
$arguments = "`"$Project`" -game `"-UghShot=$folder`" -UghShotMenu -UghShotLevels=$Levels -UghShotAt=$At `"-abslog=$log`""
$code = Invoke-UghOffscreen $UeEditor $arguments $TimeoutSeconds
$minutes = ((Get-Date) - $started).TotalMinutes
if ($code -ne 0) {
    Write-Host "FAILED: exit code $code (-1: no end in $TimeoutSeconds s), see $log" -ForegroundColor Red
    exit 1
}

# a sheet a mode: the screenshots in rows, each with its name below
$columns = 9
$width = 384
$height = 216
$label = 22
$missing = New-Object System.Collections.ArrayList
foreach ($mode in $expected.Keys) {
    $list = $expected[$mode]
    $rows = [int][math]::Ceiling($list.Count / $columns)
    $sheet = New-Object System.Drawing.Bitmap -ArgumentList ($columns * $width), ($rows * ($height + $label))
    $graphics = [System.Drawing.Graphics]::FromImage($sheet)
    $graphics.Clear([System.Drawing.Color]::Black)
    $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $font = New-Object System.Drawing.Font -ArgumentList 'Segoe UI', 11
    for ($i = 0; $i -lt $list.Count; $i++) {
        $name = '{0}-{1:D2}' -f $mode, $list[$i]
        $file = Join-Path $folder "$name.png"
        $x = ($i % $columns) * $width
        $y = [int][math]::Floor($i / $columns) * ($height + $label)
        if (Test-Path $file) {
            $image = [System.Drawing.Image]::FromFile($file)
            $graphics.DrawImage($image, $x, $y, $width, $height)
            $image.Dispose()
        } else {
            $null = $missing.Add($name)
            $name += ' MISSING'
        }
        $graphics.DrawString($name, $font, [System.Drawing.Brushes]::White, $x + 4, $y + $height + 2)
    }
    $sheetFile = Join-Path $folder "levels-$mode.png"
    $sheet.Save($sheetFile, [System.Drawing.Imaging.ImageFormat]::Png)
    $font.Dispose()
    $graphics.Dispose()
    $sheet.Dispose()
    Write-Host $sheetFile
}
if (-not (Test-Path (Join-Path $folder 'menu.png'))) { $null = $missing.Add('menu') }
if ($missing.Count -gt 0) {
    Write-Host "FAILED: no screenshot of $($missing -join ', '), see $log" -ForegroundColor Red
    exit 1
}
Write-Host ('OK: {0:N1} min' -f $minutes) -ForegroundColor Green
exit 0
