# Plays a level by itself without a window (-RenderOffscreen) and saves one screenshot to Saved\Shots\<mode>-<NN>.png:
# the game is started from the menu (the mode, the level's password), the copters hover, the screenshot is taken -At
# seconds after the level is fully shown. A check that the scene is built and the logic runs. Windows PowerShell 5.1:
#   powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\5_remake\game\shot.ps1 -Level 12 -Team
# -Level: from 1 in the order of the mode (one player 1 .. 69, -Team 1 .. 81); -Commands: console commands before the
# play, separated by commas (e.g. "r.Shadow.Virtual.Enable 0"); -Cargo <look>: the copters shown with a passenger of
# the logic's cargo look in the cabin (1 .. 4: 4 the stone, smaller), -Hanging below instead, -CloseUp: framing the
# copters (the name of the shot ends in -cargo<look> / -hanging<look>, -closeup), -Frame <left>,<top>,<width>,<height>:
# framing that part of the screen in pixels (a look at the figures; with -CloseUp from the first copter's corner: a look
# into its cabin; the name ends in -frame<left>_<top>). All levels at once:
# levels.ps1.
param([int]$Level = 1, [switch]$Team, [double]$At = 2, [string]$Commands = '', [int]$Cargo = 0, [switch]$Hanging,
    [switch]$CloseUp, [string]$Frame = '', [int]$TimeoutSeconds = 300)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'ue.ps1')
$mode = if ($Team) { 'team' } else { '1p' }
$folder = Join-Path $PSScriptRoot 'Saved\Shots'
$suffix = ''
if ($Cargo -gt 0) { $suffix += '-{0}{1}' -f $(if ($Hanging) { 'hanging' } else { 'cargo' }), $Cargo }
if ($CloseUp) { $suffix += '-closeup' }
if ($Frame) {
    $corner = $Frame.Split(',')
    if ($corner.Count -ne 4) { Write-Host "-Frame wants <left>,<top>,<width>,<height>" -ForegroundColor Red; exit 1 }
    $suffix += '-frame{0}_{1}' -f $corner[0], $corner[1]
}
$shot = Join-Path $folder ('{0}-{1:D2}{2}.png' -f $mode, $Level, $suffix)
$log = Join-Path $PSScriptRoot 'Saved\Logs\UghShot.log'
if (Test-Path $shot) { Remove-Item -Force $shot }
$arguments = "`"$Project`" -game `"-UghShot=$folder`" -UghShotLevels=${mode}:$Level -UghShotAt=$At `"-abslog=$log`""
if ($Commands) { $arguments += " `"-ExecCmds=$Commands`"" }
if ($Cargo -gt 0) { $arguments += " -UghShotCargo=$Cargo" }
if ($Hanging) { $arguments += ' -UghShotHanging' }
if ($CloseUp) { $arguments += ' -UghShotCloseUp' }
if ($Frame) { $arguments += " -UghShotFrame=$Frame" }
$code = Invoke-UghOffscreen $UeEditor $arguments $TimeoutSeconds
if ($code -ne 0 -or -not (Test-Path $shot)) {
    Write-Host "FAILED: exit code $code (-1: no end in $TimeoutSeconds s), screenshot: $(Test-Path $shot), see $log" -ForegroundColor Red
    exit 1
}
Write-Host "OK: $shot" -ForegroundColor Green
exit 0
