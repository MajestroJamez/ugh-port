# Runs a Ghidra post-script against the already analyzed UGH project (no re-import).
# Usage: powershell -ExecutionPolicy Bypass -File run-ghidra-script.ps1 <ScriptName.java> [scriptArgs...]
# NOTE: Ghidra 12.1.4 bundles Felix 7.0.5, which fails on JDK 25 -> force JDK 23.
param([Parameter(Mandatory = $true)][string]$Script, [Parameter(ValueFromRemainingArguments = $true)][string[]]$ScriptArgs)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent (Split-Path -Parent $PSCommandPath)
$env:JAVA_HOME = Join-Path $env:USERPROFILE '.jdks\temurin-23.0.2'
if (-not (Test-Path (Join-Path $env:JAVA_HOME 'bin\java.exe'))) { Write-Error "JDK 23 not found at $env:JAVA_HOME"; exit 1 }
if (-not $ScriptArgs) { $ScriptArgs = @(Join-Path $root 'out') }

$log = Join-Path $root 'out\headless-script.log'
& 'C:\Apps\ghidra\support\analyzeHeadless.bat' (Join-Path $root 'ghidra-project') UGH -process UGH.EXE -noanalysis `
    -scriptPath (Join-Path $root 'ghidra-scripts') -postScript $Script @ScriptArgs *> $log
$code = $LASTEXITCODE
Select-String -Path $log -Pattern 'ERROR|Exception|> ' | ForEach-Object { $_.Line }
if ($code -ne 0) { Write-Host "Ghidra failed (exit $code), see $log"; exit $code }
Write-Host "OK"
