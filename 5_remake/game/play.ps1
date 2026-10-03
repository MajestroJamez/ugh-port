# Plays the game in a window (the editor's game mode, no packaging); returns when the game is closed.
# Windows PowerShell 5.1:
#   powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\5_remake\game\play.ps1
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'ue.ps1')
$log = Join-Path $PSScriptRoot 'Saved\Logs\UghPlay.log'
$arguments = "`"$Project`" -game -windowed -ResX=1280 -ResY=800 `"-abslog=$log`""
$process = Start-Process -FilePath $UeEditor -ArgumentList $arguments -Wait -PassThru
if ($process.ExitCode -ne 0) { Write-Host "FAILED (exit code $($process.ExitCode)), see $log" -ForegroundColor Red; exit $process.ExitCode }
Write-Host 'OK' -ForegroundColor Green
exit 0
