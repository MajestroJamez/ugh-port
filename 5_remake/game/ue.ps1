# Where Unreal Engine 5.8 is, for the scripts of this folder (dot-sourced). Another place: $env:UE_ROOT.
$UeRoot = if ($env:UE_ROOT) { $env:UE_ROOT } else { 'C:\Program Files\Epic Games\UE_5.8' }
$UeEditor = Join-Path $UeRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$UeEditorCmd = Join-Path $UeRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$UeBuild = Join-Path $UeRoot 'Engine\Build\BatchFiles\Build.bat'
$Project = Join-Path $PSScriptRoot 'UghGame.uproject'
if (-not (Test-Path $UeEditor)) { Write-Host "Unreal Engine not found in $UeRoot (set UE_ROOT)" -ForegroundColor Red; exit 1 }

# Runs the game without a window (-RenderOffscreen) and waits for its end: $Exe is the editor (with the project and
# -game in $Arguments) or a packaged game. Returns the exit code, -1 when it did not end in $TimeoutSeconds.
function Invoke-UghOffscreen([string]$Exe, [string]$Arguments, [int]$TimeoutSeconds) {
    $process = Start-Process -FilePath $Exe -ArgumentList "$Arguments -RenderOffscreen -unattended -nosplash -nosound" -PassThru
    $null = $process.Handle   # PS 5.1 keeps the exit code only once the handle was read
    if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
        Stop-Process -Id $process.Id -Force
        return -1
    }
    return $process.ExitCode
}

# The quick set of levels.ps1 -Quick (also recorded by pso.ps1): one player 1 (day), 3 (evening), 6 (night), 8 (rising
# water), 12, 23 (dusk), 36 (high water), 43 (a storm, rising water), 62 (a storm); the team's 1, 21 and 54 (a storm).
$UghQuickLevels = '1p:1,1p:3,1p:6,1p:8,1p:12,1p:23,1p:36,1p:43,1p:62,team:1,team:21,team:54'

# Writes a profile (FUghProfile's JSON) at $Path with only a quality preset (Low, Medium, High, Epic), for a run of the
# autopilot with -UghProfile (which never saves it).
function New-UghQualityProfile([string]$Path, [string]$Quality) {
    if (@('Low', 'Medium', 'High', 'Epic') -notcontains $Quality) { throw "$Quality is not Low, Medium, High or Epic" }
    New-Item -ItemType Directory -Force (Split-Path $Path) | Out-Null
    [IO.File]::WriteAllText($Path, "{ `"version`": 1, `"settings`": { `"quality`": `"$Quality`" } }")
}
