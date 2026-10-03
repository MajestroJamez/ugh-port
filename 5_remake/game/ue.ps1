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
