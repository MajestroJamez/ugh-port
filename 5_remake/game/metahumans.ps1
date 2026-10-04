# Makes the game's MetaHumans (UghMetaHumans.h) with MetaHuman Creator in the editor: the pilot and the passengers
# of the cargo looks 1 .. 3 from Creator's presets, dressed as stone age people, assembled into
# Content\External\MetaHumans (Python\metahumans.py; not in git: Epic's content, licensed for Unreal Engine projects),
# then their actions (Python\metahuman_actions.py). Needs the plugin's optional content ("MetaHuman Creator Core
# Data", Fab / Epic Games Launcher) and the editor logged in to an Epic account (the face rig and the skin textures come
# from Epic's cloud). The editor runs without a window. Windows PowerShell 5.1:
#   powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\5_remake\game\metahumans.ps1
# -All makes the characters again (else only the missing ones; the actions always). Run build.ps1 before. Without
# the MetaHumans the game shows the caveman of Blender\caveman.py.
param([switch]$All)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'ue.ps1')
$logs = Join-Path $PSScriptRoot 'Saved\Logs'
New-Item -ItemType Directory -Force $logs | Out-Null

# Runs a Python script and checks its report (its last line OK).
function Invoke-UghPython([string]$Exe, [string]$Arguments, [string]$Report, [string]$What) {
    if (Test-Path $Report) { Remove-Item $Report }
    $process = Start-Process -FilePath $Exe -ArgumentList "$Project $Arguments -unattended -nopause -nosplash -nosound" -PassThru
    $null = $process.Handle   # PS 5.1 keeps the exit code only once the handle was read
    if (-not $process.WaitForExit(3 * 3600 * 1000)) {
        Stop-Process -Id $process.Id -Force
        Write-Host "FAILED: $What (timeout)" -ForegroundColor Red; exit 1
    }
    if (-not (Test-Path $Report)) { Write-Host "FAILED: $What (no report $Report)" -ForegroundColor Red; exit 1 }
    $lines = @(Get-Content $Report)
    $lines | ForEach-Object { Write-Host $_ }
    if ($lines[-1] -ne 'OK') { Write-Host "FAILED: $What, see $Report" -ForegroundColor Red; exit 1 }
}

$script = Join-Path $PSScriptRoot 'Python\metahumans.py'
$flag = if ($All) { ' -UghAll' } else { '' }
Invoke-UghPython $UeEditor "-ExecutePythonScript=`"$script`" -RenderOffscreen$flag -abslog=`"$logs\UghMetaHumans.log`"" `
    (Join-Path $logs 'UghMetaHumans.txt') 'the MetaHumans'
$script = Join-Path $PSScriptRoot 'Python\metahuman_actions.py'
Invoke-UghPython $UeEditorCmd "-run=pythonscript `"-script=$script`" -abslog=`"$logs\UghMetaHumanActions.log`"" `
    (Join-Path $logs 'UghMetaHumanActions.txt') 'their actions'
Write-Host 'OK' -ForegroundColor Green
exit 0
