# Runs the automation tests inside Unreal Engine without a window: the golden replays (Ugh.Replays.*) and the menu
# (Ugh.Menu). -Filter: other tests (a part of the name; several joined by +). Windows PowerShell 5.1:
#   powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\5_remake\game\test.ps1
# Needs build.ps1, the data (.\gradlew.bat :extractor:run) and the replays (.\gradlew.bat :verify:replays).
param([string]$Filter = 'Ugh.')

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'ue.ps1')
$report = Join-Path $PSScriptRoot 'Saved\TestReport'
if (Test-Path $report) { Remove-Item -Recurse -Force $report }
& $UeEditorCmd $Project -nullrhi -unattended -nopause -nosplash -nosound -NoLogTimes `
    "-ExecCmds=Automation RunTests $Filter; Quit" "-TestExit=Automation Test Queue Empty" "-ReportExportPath=$report" `
    "-abslog=$PSScriptRoot\Saved\Logs\UghTests.log" | Out-Null
$code = $LASTEXITCODE
$index = Join-Path $report 'index.json'
if (-not (Test-Path $index)) { Write-Host "FAILED: no test report (exit code $code), see Saved\Logs\UghTests.log" -ForegroundColor Red; exit 1 }
# the report is UTF-8 with a BOM
$json = [System.IO.File]::ReadAllText($index).TrimStart([char]0xFEFF) | ConvertFrom-Json
$failed = @($json.tests | Where-Object { $_.state -ne 'Success' })
foreach ($test in $failed) {
    Write-Host "$($test.fullTestPath): $($test.state)" -ForegroundColor Red
    foreach ($entry in $test.entries) { if ($entry.event.type -eq 'Error') { Write-Host "  $($entry.event.message)" } }
}
Write-Host "$($json.succeeded) passed, $($json.failed) failed, $($json.notRun) not run"
if ($failed.Count -gt 0 -or $json.succeeded -eq 0) { Write-Host 'FAILED' -ForegroundColor Red; exit 1 }
Write-Host 'OK' -ForegroundColor Green
exit 0
