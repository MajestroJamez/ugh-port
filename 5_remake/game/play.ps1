# Plays the game in a window; returns when the game is closed. Windows PowerShell 5.1:
#   powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\5_remake\game\play.ps1
# The packaged game (package.ps1 / pso.ps1: cooked, with its PSO cache - smooth from the first frame) when there is
# one, else (or with -Editor) the editor's game mode: uncooked, its shaders compiled while it plays (it stutters and
# is slower), but always the code and content as they are now. A package older than the last build is said. Both
# keep the same profile (the settings, the keys, the high scores): Saved\UghProfile.json; the log Saved\Logs\UghPlay.log.
param([switch]$Editor)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'ue.ps1')
$packaged = Join-Path $PSScriptRoot 'Packaged\Windows\UghGame.exe'
$log = Join-Path $PSScriptRoot 'Saved\Logs\UghPlay.log'
$profilePath = Join-Path $PSScriptRoot 'Saved\UghProfile.json'
$common = "-windowed -ResX=1280 -ResY=800 `"-UghProfile=$profilePath`" `"-abslog=$log`""
if (-not $Editor -and (Test-Path $packaged)) {
    $built = Get-ChildItem (Join-Path $PSScriptRoot 'Binaries\Win64') -Filter 'UnrealEditor-UghGame*.dll' -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
    $pak = Get-ChildItem (Join-Path $PSScriptRoot 'Packaged\Windows\UghGame\Content\Paks') -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if ($built -and $pak -and $built.LastWriteTime -gt $pak.LastWriteTime) {
        Write-Host "the package is older than the last build ($($pak.LastWriteTime) < $($built.LastWriteTime)): pso.ps1 makes a new one, -Editor plays the build" -ForegroundColor Yellow
    }
    $process = Start-Process -FilePath $packaged -ArgumentList $common -Wait -PassThru
} else {
    $process = Start-Process -FilePath $UeEditor -ArgumentList "`"$Project`" -game $common" -Wait -PassThru
}
if ($process.ExitCode -ne 0) { Write-Host "FAILED (exit code $($process.ExitCode)), see $log" -ForegroundColor Red; exit $process.ExitCode }
Write-Host 'OK' -ForegroundColor Green
exit 0
