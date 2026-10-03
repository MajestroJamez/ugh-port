# Plays a level by itself without a window (-RenderOffscreen) and saves one screenshot to Saved\Shots\<mode>-<NN>.png:
# the game is started from the menu (the mode, the level's password), the copters hover, the screenshot is taken -At
# seconds after the level is fully shown. A check that the scene is built and the logic runs. Windows PowerShell 5.1:
#   powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\5_remake\game\shot.ps1 -Level 12 -Team
# -Level: from 1 in the order of the mode (one player 1 .. 69, -Team 1 .. 81); -Commands: console commands before the
# play, separated by commas (e.g. "r.Shadow.Virtual.Enable 0"). All levels at once: levels.ps1.
param([int]$Level = 1, [switch]$Team, [double]$At = 2, [string]$Commands = '', [int]$TimeoutSeconds = 300)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'ue.ps1')
$mode = if ($Team) { 'team' } else { '1p' }
$folder = Join-Path $PSScriptRoot 'Saved\Shots'
$shot = Join-Path $folder ('{0}-{1:D2}.png' -f $mode, $Level)
$log = Join-Path $PSScriptRoot 'Saved\Logs\UghShot.log'
if (Test-Path $shot) { Remove-Item -Force $shot }
$arguments = "`"$Project`" -game `"-UghShot=$folder`" -UghShotLevels=${mode}:$Level -UghShotAt=$At `"-abslog=$log`""
if ($Commands) { $arguments += " `"-ExecCmds=$Commands`"" }
$code = Invoke-UghOffscreen $UeEditor $arguments $TimeoutSeconds
if ($code -ne 0 -or -not (Test-Path $shot)) {
    Write-Host "FAILED: exit code $code (-1: no end in $TimeoutSeconds s), screenshot: $(Test-Path $shot), see $log" -ForegroundColor Red
    exit 1
}
Write-Host "OK: $shot" -ForegroundColor Green
exit 0
