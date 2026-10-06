# Plays a level by itself without a window (-RenderOffscreen) and saves one screenshot to Saved\Shots\<mode>-<NN>.png:
# the game is started from the menu (the mode, the level's password), the copters hover, the screenshot is taken -At
# seconds after the level is fully shown. A check that the scene is built and the logic runs. Windows PowerShell 5.1:
#   powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\5_remake\game\shot.ps1 -Level 12 -Team
# -Level: from 1 in the order of the mode (one player 1 .. 69, -Team 1 .. 81); -Commands: console commands before the
# play, separated by commas (e.g. "r.Shadow.Virtual.Enable 0"); -Cargo <look>: the copters shown with a passenger of
# the logic's cargo look in the cabin (1 .. 4: 4 the stone, smaller), -Hanging below instead, -Land: the copters come
# down slowly instead of hovering until they stand on the ground below (give it -At 8), -Bubbles: every passenger
# with a speech bubble (each the next of the data's), -CloseUp: framing the copters (the name of the shot ends in
# -cargo<look> / -hanging<look>, -landed, -bubbles, -closeup), -Look campfire|torch: framing the first campfire (torch) of the
# level (the name ends in -campfire / -torch), -Frame <left>,<top>,<width>,<height>: framing that part of the screen in
# pixels (a look at the figures; with -CloseUp from the first copter's corner: a look into its cabin, with -Look from
# the middle of the campfire or torch; the name ends in -frame<left>_<top>), -Intro <seconds>: the flight to the stone
# at the start of the level that many seconds into it instead (4.5: its end, the game's camera; later: that much after
# its end, the caption still shown; the name ends in -intro<seconds>, e.g. -intro0.3), -Effect <bursts>: a burst of the
# events (names of UghBursts.cpp separated by commas, or all) held by the first copter, framed around it, one shot each
# (the name ends in -<burst>, e.g. 1p-01-explosion), -EffectAge <seconds> into it (else its own moment), -Edge
# left|right|top: the first copter flies into that edge of the screen (beside it at the height -EdgeY pixels, 20 without
# it; the top pedalling up) and the shot is taken -EdgeAfter seconds (0.25) after it got there: the plants of the soft
# edges bent (the name ends in -edge<edge> after -frame...), -End: the level given up instead and the card of the game's
# end in the menu shot (the name ends in -end; -Score <points>: the game ended with so many points, a high score shows
# its name being typed), -Menu: the title screen first too (menu.png), -Screens settings,controls,scores: those screens
# of the menu first too (<screen>.png). The game's profile is the defaults, or -Profile <file> (a JSON of FUghProfile);
# with -Screens scores or -Score and without -Profile a sample one with high scores (Saved\Shots\profile-sample.json).
# The shots show the screen (the menu, the HUD) too. All levels at once: levels.ps1.
param([int]$Level = 1, [switch]$Team, [double]$At = 2, [string]$Commands = '', [int]$Cargo = 0, [switch]$Hanging,
    [switch]$Land,
    [switch]$Bubbles, [switch]$CloseUp, [string]$Look = '', [string]$Frame = '', [string]$Intro = '', [string]$Effect = '',
    [string]$EffectAge = '', [string]$Edge = '', [double]$EdgeAfter = -1, [int]$EdgeY = -1, [switch]$End,
    [int]$Score = 0, [switch]$Menu, [string]$Screens = '', [string]$Profile = '',
    [int]$TimeoutSeconds = 300)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'ue.ps1')
$mode = if ($Team) { 'team' } else { '1p' }
$folder = Join-Path $PSScriptRoot 'Saved\Shots'
$suffix = ''
if ($Cargo -gt 0) { $suffix += '-{0}{1}' -f $(if ($Hanging) { 'hanging' } else { 'cargo' }), $Cargo }
if ($Land) { $suffix += '-landed' }
if ($Bubbles) { $suffix += '-bubbles' }
if ($CloseUp) { $suffix += '-closeup' }
if ($Look) {
    if ($Look -ne 'campfire' -and $Look -ne 'torch') {
        Write-Host "-Look wants campfire or torch" -ForegroundColor Red; exit 1
    }
    $suffix += '-' + $Look
}
if ($Frame) {
    $corner = $Frame.Split(',')
    if ($corner.Count -ne 4) { Write-Host "-Frame wants <left>,<top>,<width>,<height>" -ForegroundColor Red; exit 1 }
    $suffix += '-frame{0}_{1}' -f $corner[0], $corner[1]
}
if ($Edge) {
    if (@('left', 'right', 'top') -notcontains $Edge) { Write-Host "-Edge wants left, right or top" -ForegroundColor Red; exit 1 }
    $suffix += '-edge' + $Edge
}
if ($Intro) {
    # the seconds as the game writes them (invariant: a dot, no trailing zeros)
    $invariant = [Globalization.CultureInfo]::InvariantCulture
    $seconds = 0.0
    if (-not [double]::TryParse($Intro, [Globalization.NumberStyles]::Float, $invariant, [ref]$seconds)) {
        Write-Host "-Intro wants seconds (e.g. 2.5)" -ForegroundColor Red; exit 1
    }
    $Intro = $seconds.ToString('G', $invariant)
    $suffix += '-intro' + $Intro
}
if ($End) { $suffix += '-end' }
$shot = Join-Path $folder ('{0}-{1:D2}{2}.png' -f $mode, $Level, $suffix)
$menuShot = Join-Path $folder 'menu.png'
$log = Join-Path $PSScriptRoot 'Saved\Logs\UghShot.log'
$screenShots = @()
if ($Screens) {
    foreach ($screen in $Screens.Split(',')) {
        if (@('settings', 'controls', 'scores') -notcontains $screen) {
            Write-Host "-Screens wants settings, controls, scores separated by commas" -ForegroundColor Red; exit 1
        }
        $screenShots += Join-Path $folder "$screen.png"
    }
}
if (-not $Profile -and ($Score -gt 0 -or $screenShots -match 'scores\.png$')) {
    # a sample profile: high scores in both modes (one player's table not full), the levels the last games got to
    $Profile = Join-Path $folder 'profile-sample.json'
    $json = @'
{ "version": 1, "highScores": {
  "onePlayer": { "lastLevel": 22, "best": [
    { "name": "GROG", "score": 9840, "level": 23, "difficulty": 2, "day": "2026-10-05" },
    { "name": "ZUGZUG", "score": 7310, "level": 17, "difficulty": 1, "day": "2026-10-04" },
    { "name": "KRAA", "score": 6050, "level": 14, "difficulty": 1, "day": "2026-10-06" },
    { "name": "OOK", "score": 4620, "level": 11, "difficulty": 0, "day": "2026-10-03" },
    { "name": "MAMMOTH", "score": 3900, "level": 9, "difficulty": 1, "day": "2026-10-05" },
    { "name": "THAG", "score": 2480, "level": 6, "difficulty": 1, "day": "2026-10-02" },
    { "name": "RAWR", "score": 1270, "level": 4, "difficulty": 0, "day": "2026-10-06" },
    { "name": "NUK", "score": 640, "level": 2, "difficulty": 0, "day": "2026-10-01" } ] },
  "team": { "lastLevel": 40, "best": [
    { "name": "GROG + UGH", "score": 15200, "level": 37, "difficulty": 2, "day": "2026-10-05" },
    { "name": "BORK", "score": 12650, "level": 31, "difficulty": 1, "day": "2026-10-04" },
    { "name": "KRAA", "score": 11020, "level": 28, "difficulty": 1, "day": "2026-10-03" },
    { "name": "ZUG", "score": 9480, "level": 24, "difficulty": 1, "day": "2026-10-06" },
    { "name": "OOK OOK", "score": 8100, "level": 21, "difficulty": 0, "day": "2026-10-02" },
    { "name": "LUNA", "score": 6730, "level": 18, "difficulty": 1, "day": "2026-10-05" },
    { "name": "THAG", "score": 5300, "level": 14, "difficulty": 2, "day": "2026-10-01" },
    { "name": "RAWR", "score": 3990, "level": 11, "difficulty": 1, "day": "2026-10-04" },
    { "name": "NUK", "score": 2150, "level": 7, "difficulty": 0, "day": "2026-10-03" },
    { "name": "UGH", "score": 980, "level": 3, "difficulty": 0, "day": "2026-10-06" } ] },
  "lastName": "GROG" } }
'@
    New-Item -ItemType Directory -Force $folder | Out-Null
    [IO.File]::WriteAllText($Profile, $json)
}
$fresh = @($shot) + $screenShots
if ($Menu) { $fresh += $menuShot }
foreach ($old in $fresh) { if (Test-Path $old) { Remove-Item -Force $old } }
$arguments = "`"$Project`" -game `"-UghShot=$folder`" -UghShotLevels=${mode}:$Level -UghShotAt=$At `"-abslog=$log`""
if ($Commands) { $arguments += " `"-ExecCmds=$Commands`"" }
if ($Cargo -gt 0) { $arguments += " -UghShotCargo=$Cargo" }
if ($Hanging) { $arguments += ' -UghShotHanging' }
if ($Land) { $arguments += ' -UghShotLand' }
if ($Bubbles) { $arguments += ' -UghShotBubbles' }
if ($CloseUp) { $arguments += ' -UghShotCloseUp' }
if ($Look) { $arguments += " -UghShotLook=$Look" }
if ($Frame) { $arguments += " -UghShotFrame=$Frame" }
if ($Edge) { $arguments += " -UghShotEdge=$Edge" }
if ($EdgeAfter -ge 0) { $arguments += " -UghShotEdgeAfter=" + $EdgeAfter.ToString([Globalization.CultureInfo]::InvariantCulture) }
if ($EdgeY -ge 0) { $arguments += " -UghShotEdgeY=$EdgeY" }
if ($Intro) { $arguments += " -UghShotIntro=$Intro" }
if ($Effect) { $arguments += " -UghShotEffect=$Effect" }
if ($EffectAge) { $arguments += " -UghShotEffectAge=$EffectAge" }
if ($End) { $arguments += ' -UghShotEnd' }
if ($Menu) { $arguments += ' -UghShotMenu' }
if ($Score -gt 0) { $arguments += " -UghShotScore=$Score" }
if ($Screens) { $arguments += " -UghShotScreens=$Screens" }
if ($Profile) { $arguments += " `"-UghProfile=$Profile`"" }
$code = Invoke-UghOffscreen $UeEditor $arguments $TimeoutSeconds
# the shots the game says it took (one a burst), else the one
$shots = @($shot)
if ($Effect -and (Test-Path $log)) {
    $shots = @(Select-String -Path $log -Pattern 'UGH shot (.+\.png)$' | ForEach-Object { $_.Matches[0].Groups[1].Value })
}
if ($Menu) { $shots += $menuShot }
$shots += $screenShots
$wanted = if ($Effect -eq 'all') { 1 } elseif ($Effect) { $Effect.Split(',').Count } else { 1 }
$missing = @($shots | Where-Object { -not (Test-Path $_) })
if ($code -ne 0 -or $shots.Count -lt $wanted -or $missing.Count -gt 0) {
    Write-Host "FAILED: exit code $code (-1: no end in $TimeoutSeconds s), $($shots.Count) screenshots, missing: $missing, see $log" -ForegroundColor Red
    exit 1
}
foreach ($each in $shots) { Write-Host "OK: $each" -ForegroundColor Green }
exit 0
