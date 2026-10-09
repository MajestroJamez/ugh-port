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
# (the name ends in -<burst>, e.g. 1p-01-explosion), -EffectAge <seconds> into it (else its own moment), -EffectAt
# <x>,<y> held at that place of the screen instead (pixels; on the ground under it, the water below it; the name ends
# in -at<x>_<y>), -Wide seen from the game's camera, not framed (-wide), -Shake: the camera shaken by an impact as in
# the play (else the shots stand still; the log has its offsets), -Edge
# left|right|top: the first copter flies into that edge of the screen (beside it at the height -EdgeY pixels, 20 without
# it; the top pedalling up) and the shot is taken -EdgeAfter seconds (0.25) after it got there: the plants of the soft
# edges bent (the name ends in -edge<edge> after -frame...), -Fling <seconds>: the first copter knocks the first
# passenger on land off its pad and the shot is taken that many seconds after the knock (several separated by commas,
# one shot each; the passenger flung towards the camera into the sea, the name ends in -fling<seconds>, e.g.
# 1p-01-fling0.5), -Dunk <seconds>: the first copter flies over open water and falls into the sea, the shot taken that many
# seconds after its waterline met the surface (negative: before it; several separated by commas, one shot each; the
# name ends in -dunk<seconds>, e.g. 1p-08-dunk0.2), -Drop <seconds>: the first copter takes the stone on its sling, flies
# above the first enemy and lets it go, the shot taken that many seconds after the logic let it fall (tumbling down,
# bouncing off the enemy; several separated by commas, one shot each; the name ends in -drop<seconds>, e.g.
# 1p-01-drop0.3), -Rush left|right|down: the first copter climbs to the height -RushY pixels (40 without it), then flies that
# way as fast as it can (sideways keeping its height, down diving), the shot taken -RushAfter seconds after it began (several
# separated by commas, one shot each; the name ends in -rush<way><seconds>, e.g. 1p-01-rushright1.2), -Difficulty 0|1|2:
# the game at easy, medium (the default) or hard, -End: the level given up instead and the card of the game's end in the menu shot (the name ends in
# -end; -Score <points>: the game ended with so many points, a high score shows its name being typed), -Menu: the
# title screen first too (menu.png), -Screens settings,controls,scores: those screens of the menu first too
# (<screen>.png). The game's profile is the defaults, or -Profile <file> (a JSON of FUghProfile);
# with -Screens scores, -Score or -Isles and without -Profile a sample one with high scores and levels done
# (Saved\Shots\profile-sample.json: one player up to level 22, the team up to 40). -Isles <moments>: the level
# selection opened on the way to the level and shot at those moments (separated by commas: over:<seconds> into the
# flight over the archipelago, choose - the cursor moved onto the level's stone -, approach:<seconds> into the flight to
# it, arrive at its end; the names end in -isles-over2, -isles-choose, -isles-approach1.5, -isles-arrive), then the
# level as ever.
# The shots show the screen (the menu, the HUD) too. All levels at once: levels.ps1.
param([int]$Level = 1, [switch]$Team, [double]$At = 2, [string]$Commands = '', [int]$Cargo = 0, [switch]$Hanging,
    [switch]$Land,
    [switch]$Bubbles, [switch]$CloseUp, [string]$Look = '', [string]$Frame = '', [string]$Intro = '', [string]$Effect = '',
    [string]$EffectAge = '', [string]$EffectAt = '', [switch]$Wide, [switch]$Shake, [string]$Fling = '', [string]$Dunk = '', [string]$Drop = '', [string]$Rush = '', [string]$RushAfter = '1', [int]$RushY = -1, [int]$Difficulty = -1, [string]$Edge = '', [double]$EdgeAfter = -1, [int]$EdgeY = -1, [switch]$End,
    [int]$Score = 0, [switch]$Menu, [string]$Screens = '', [string]$Profile = '', [string]$Isles = '',
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
if (-not $Profile -and ($Score -gt 0 -or $Isles -or $screenShots -match 'scores\.png$')) {
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
if ($EffectAt) { $arguments += " -UghShotEffectAt=$EffectAt" }
if ($Wide) { $arguments += ' -UghShotWide' }
if ($Shake) { $arguments += ' -UghShotShake' }
if ($Fling) { $arguments += " -UghShotFling=$Fling" }
if ($Dunk) { $arguments += " -UghShotDunk=$Dunk" }
if ($Drop) { $arguments += " -UghShotDrop=$Drop" }
if ($Rush) { $arguments += " -UghShotRush=$Rush -UghShotRushAfter=$RushAfter" }
if ($RushY -ge 0) { $arguments += " -UghShotRushY=$RushY" }
if ($Difficulty -ge 0) { $arguments += " -UghShotDifficulty=$Difficulty" }
if ($End) { $arguments += ' -UghShotEnd' }
if ($Menu) { $arguments += ' -UghShotMenu' }
if ($Score -gt 0) { $arguments += " -UghShotScore=$Score" }
if ($Screens) { $arguments += " -UghShotScreens=$Screens" }
if ($Isles) { $arguments += " -UghShotIsles=$Isles" }
if ($Profile) { $arguments += " `"-UghProfile=$Profile`"" }
$code = Invoke-UghOffscreen $UeEditor $arguments $TimeoutSeconds
# the shots the game says it took (one a burst), else the one
$shots = @($shot)
if (($Effect -or $Fling -or $Dunk -or $Drop -or $Rush -or $Isles) -and (Test-Path $log)) {
    $shots = @(Select-String -Path $log -Pattern 'UGH shot (.+\.png)$' | ForEach-Object { $_.Matches[0].Groups[1].Value })
}
if ($Menu) { $shots += $menuShot }
$shots += $screenShots
$wanted = if ($Effect -eq 'all') { 1 } elseif ($Effect) { $Effect.Split(',').Count } else { 1 }
if ($Fling) { $wanted *= $Fling.Split(',').Count }
if ($Dunk) { $wanted *= $Dunk.Split(',').Count }
if ($Drop) { $wanted *= $Drop.Split(',').Count }
if ($Rush) { $wanted *= $RushAfter.Split(",").Count }
if ($Isles) { $wanted += $Isles.Split(',').Count }
$missing = @($shots | Where-Object { -not (Test-Path $_) })
if ($code -ne 0 -or $shots.Count -lt $wanted -or $missing.Count -gt 0) {
    Write-Host "FAILED: exit code $code (-1: no end in $TimeoutSeconds s), $($shots.Count) screenshots, missing: $missing, see $log" -ForegroundColor Red
    exit 1
}
foreach ($each in $shots) { Write-Host "OK: $each" -ForegroundColor Green }
exit 0
