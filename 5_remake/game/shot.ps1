# Plays level 1 by itself without a window (-RenderOffscreen) and saves one screenshot to Saved\Shots\<Name>.png:
# a check that the scene is built and the logic runs. Windows PowerShell 5.1:
#   powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\5_remake\game\shot.ps1
# -Commands: console commands before the play, separated by commas (e.g. "r.Shadow.Virtual.Enable 0").
param([string]$Commands = '', [string]$Name = 'level1', [int]$TimeoutSeconds = 300)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'ue.ps1')
$shot = Join-Path $PSScriptRoot "Saved\Shots\$Name.png"
$log = Join-Path $PSScriptRoot 'Saved\Logs\UghShot.log'
if (Test-Path $shot) { Remove-Item -Force $shot }
$arguments = "`"$Project`" -game -RenderOffscreen -unattended -nosplash -nosound `"-UghShot=$shot`" `"-abslog=$log`""
if ($Commands) { $arguments += " `"-ExecCmds=$Commands`"" }
$process = Start-Process -FilePath $UeEditor -ArgumentList $arguments -PassThru
$null = $process.Handle   # PS 5.1 keeps the exit code only once the handle was read
if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
    Stop-Process -Id $process.Id -Force
    Write-Host "FAILED: no end after $TimeoutSeconds s, see $log" -ForegroundColor Red
    exit 1
}
if ($process.ExitCode -ne 0 -or -not (Test-Path $shot)) {
    Write-Host "FAILED: exit code $($process.ExitCode), screenshot: $(Test-Path $shot), see $log" -ForegroundColor Red
    exit 1
}
Write-Host "OK: $shot" -ForegroundColor Green
exit 0
