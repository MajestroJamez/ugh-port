# Downloads the 3D assets of Assets.json to assets\3d (never committed): only what is missing, each file checked by
# its size and hash (Poly Haven: the MD5 of its API; a download: the SHA-256 of the manifest), archives extracted,
# Blender scripts run where an asset needs one (a generated asset is made by its script alone, and made again whenever
# a script in the folder Blender is newer than it); the whole folder stays under the manifest's budget. A local asset
# is not downloaded (Jan's own models, the scans electric-dreams.ps1 exports): without its files it is absent, and so is
# what is made of it (the game shows the older model then) - no failure. Ends with a table of the assets. build.ps1
# then imports them into Content\Imported. Windows PowerShell 5.1:
#   powershell -ExecutionPolicy Bypass -File C:\Users\Ja079591\IdeaProjects\UGH\5_remake\game\fetch-assets.ps1
# -Only: some assets (ids, comma separated); -Verify: hash the files already there too. Blender: $env:BLENDER or the
# default place of Blender 5.2.
param([string]$Only = '', [switch]$Verify)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
$manifest = Get-Content -Raw (Join-Path $PSScriptRoot 'Assets.json') | ConvertFrom-Json
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$root = Join-Path $repo $manifest.folder
$archives = Join-Path $root '_archives'
$blender = if ($env:BLENDER) { $env:BLENDER } else { 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe' }
$agent = 'UGH-remake fetch-assets.ps1'

function Get-Hash([string]$Path, [string]$Algorithm) {
    return (Get-FileHash -Path $Path -Algorithm $Algorithm).Hash.ToLowerInvariant()
}

# The file at $Path is there with the size and hash wanted (the hash only when it is new or -Verify).
function Test-File([string]$Path, [long]$Size, [string]$Algorithm, [string]$Hash, [bool]$Hashing) {
    if (-not (Test-Path -LiteralPath $Path)) { return $false }
    if ((Get-Item -LiteralPath $Path).Length -ne $Size) { return $false }
    return (-not $Hashing) -or (Get-Hash $Path $Algorithm) -eq $Hash
}

# Downloads $Url to $Path through a .part file; throws when the size or hash is not the one wanted.
function Save-File([string]$Url, [string]$Path, [long]$Size, [string]$Algorithm, [string]$Hash) {
    $null = New-Item -ItemType Directory -Force (Split-Path -Parent $Path)
    $part = "$Path.part"
    $client = New-Object System.Net.WebClient
    $client.Headers.Add('User-Agent', $agent)
    try { $client.DownloadFile($Url, $part) } finally { $client.Dispose() }
    if (-not (Test-File $part $Size $Algorithm $Hash $true)) {
        Remove-Item -LiteralPath $part -Force
        throw "$Url is not the file of the manifest (size $Size, $Algorithm $Hash)"
    }
    Move-Item -LiteralPath $part -Destination $Path -Force
}

# The files of an asset: url, local path, size, hash algorithm and hash. Poly Haven: its API at the asset's resolution
# (a glTF brings the files it includes); a download: the archive or file of the manifest; a generated asset: none.
function Get-Files($Asset) {
    $folder = Join-Path $root $Asset.path
    $files = New-Object System.Collections.ArrayList
    if ($Asset.polyhaven) {
        $client = New-Object System.Net.WebClient
        $client.Headers.Add('User-Agent', $agent)
        try { $api = $client.DownloadString("https://api.polyhaven.com/files/$($Asset.polyhaven.id)") | ConvertFrom-Json }
        finally { $client.Dispose() }
        foreach ($wanted in $Asset.polyhaven.files) {
            $map, $format = $wanted.Split('.')
            $node = $api.$map.($Asset.polyhaven.resolution).$format
            if (-not $node) { throw "Poly Haven has no $wanted at $($Asset.polyhaven.resolution) for $($Asset.polyhaven.id)" }
            $null = $files.Add(@{ Url = $node.url; Path = Join-Path $folder (Split-Path -Leaf $node.url); Size = [long]$node.size; Algorithm = 'MD5'; Hash = $node.md5 })
            if ($node.include) {
                foreach ($include in $node.include.PSObject.Properties) {
                    $null = $files.Add(@{ Url = $include.Value.url; Path = Join-Path $folder $include.Name; Size = [long]$include.Value.size; Algorithm = 'MD5'; Hash = $include.Value.md5 })
                }
            }
        }
    } elseif ($Asset.download) {
        $name = [Uri]::UnescapeDataString((Split-Path -Leaf ([Uri]$Asset.download.url).AbsolutePath))
        if ($name -eq 'get') { $name = $Asset.download.url.Split('=')[-1] }   # ambientCG: get?file=<name>
        $null = $files.Add(@{ Url = $Asset.download.url; Path = Join-Path $archives $name; Size = [long]$Asset.download.size; Algorithm = 'SHA256'; Hash = $Asset.download.sha256; Archive = $true })
    }
    return ,$files
}

# What the asset must have once fetched: its maps, the files to import, a local asset's files.
function Get-Missing($Asset) {
    $folder = Join-Path $root $Asset.path
    $needed = @()
    if ($Asset.maps) { $needed += @($Asset.maps.PSObject.Properties | ForEach-Object { $_.Value }) }
    if ($Asset.import) { $needed += @($Asset.import) }
    if ($Asset.files) { $needed += @($Asset.files) }
    return @($needed | Where-Object { -not (Test-Path -LiteralPath (Join-Path $folder $_)) })
}

function Get-FolderBytes([string]$Path) {
    if (-not (Test-Path $Path)) { return [long]0 }
    $sum = (Get-ChildItem -Recurse -File $Path | Measure-Object -Sum Length).Sum
    if ($sum) { return [long]$sum } else { return [long]0 }
}

$assets = @($manifest.assets)
if ($Only) {
    $ids = $Only.Split(',') | ForEach-Object { $_.Trim() }
    $unknown = @($ids | Where-Object { $assets.id -notcontains $_ })
    if ($unknown.Count -gt 0) { Write-Host "FAILED: no asset $($unknown -join ', ') in Assets.json" -ForegroundColor Red; exit 1 }
    $assets = @($assets | Where-Object { $ids -contains $_.id })
}

# the plan: every asset's files, what is to be downloaded, and whether it fits the budget
$plan = @()
$toDownload = [long]0
foreach ($asset in $assets) {
    try {
        $files = Get-Files $asset
        $wanted = @($files | Where-Object { -not (Test-File $_.Path $_.Size $_.Algorithm $_.Hash $Verify.IsPresent) })
        foreach ($file in $wanted) { $toDownload += $file.Size }
        $plan += @{ Asset = $asset; Files = $files; Wanted = $wanted; Error = '' }
    } catch {
        $plan += @{ Asset = $asset; Files = @(); Wanted = @(); Error = "$_" }
    }
}
$budget = [long]$manifest.budgetGB * 1GB
$present = Get-FolderBytes $root
if ($present + $toDownload -gt $budget) {
    Write-Host ('FAILED: {0:N1} GB there + {1:N1} GB to download is over the budget of {2} GB' -f ($present / 1GB), ($toDownload / 1GB), $manifest.budgetGB) -ForegroundColor Red
    exit 1
}
Write-Host ('{0:N1} MB to download into {1}' -f ($toDownload / 1MB), $root)

# a local asset without its files, and what is made of one
$absent = @{}
foreach ($asset in $manifest.assets) {
    if ($asset.kind -eq 'local' -and (Get-Missing $asset).Count -gt 0) { $absent[$asset.id] = "put $($asset.path) there first ($($asset.page))" }
}
foreach ($asset in $manifest.assets) {
    $lacking = @($asset.blender.inputs | Where-Object { $_ -and $absent.ContainsKey($_) })
    if ($lacking.Count -gt 0) { $absent[$asset.id] = "needs $($lacking -join ', ')" }
}

$rows = @()
foreach ($item in $plan) {
    $asset = $item.Asset
    $folder = Join-Path $root $asset.path
    $status = if ($item.Wanted.Count -gt 0) { 'downloaded' } else { 'present' }
    $problem = $item.Error
    if ($absent.ContainsKey($asset.id)) {
        $status = 'absent'
        $problem = $absent[$asset.id]
    } elseif (-not $problem) {
        try {
            foreach ($file in $item.Wanted) {
                Write-Host "  $($asset.id): $($file.Url)"
                Save-File $file.Url $file.Path $file.Size $file.Algorithm $file.Hash
            }
            # an archive is extracted when it is new or its asset misses a file (the archive's root becomes the folder)
            $archive = @($item.Files | Where-Object { $_.Archive })
            if ($archive.Count -gt 0 -and ($item.Wanted.Count -gt 0 -or (Get-Missing $asset).Count -gt 0)) {
                $null = New-Item -ItemType Directory -Force $folder
                & "$env:SystemRoot\System32\tar.exe" -xf $archive[0].Path -C $folder
                if ($LASTEXITCODE -ne 0) { throw "tar could not extract $($archive[0].Path) (exit code $LASTEXITCODE)" }
                if ($status -eq 'present') { $status = 'extracted' }
            }
            if ($asset.blender) {
                # made again when an output is missing; a generated asset also when a script of Blender\ is newer
                # than an output (the scripts share modules)
                $made = @($asset.blender.outputs | ForEach-Object { Join-Path $folder $_ })
                $stale = @($made | Where-Object { -not (Test-Path -LiteralPath $_) })
                if ($asset.kind -eq 'generated' -and $stale.Count -eq 0) {
                    $newest = (Get-ChildItem (Join-Path $PSScriptRoot 'Blender\*.py') | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1).LastWriteTimeUtc
                    $stale = @($made | Where-Object { (Get-Item -LiteralPath $_).LastWriteTimeUtc -lt $newest })
                }
                if ($stale.Count -gt 0) {
                    if (-not (Test-Path $blender)) { throw "needs Blender ($blender; set BLENDER) for $($asset.blender.script)" }
                    $script = Join-Path $PSScriptRoot $asset.blender.script
                    $null = New-Item -ItemType Directory -Force $folder
                    $log = Join-Path $folder 'blender.log'
                    $arguments = "-b --factory-startup --python-exit-code 1 --python `"$script`" -- `"$folder`""
                    foreach ($id in @($asset.blender.inputs | Where-Object { $_ })) {
                        $source = @($manifest.assets | Where-Object { $_.id -eq $id })
                        if ($source.Count -ne 1) { throw "$($asset.blender.script) needs asset $id, which Assets.json does not have" }
                        $arguments += " `"$(Join-Path $root $source[0].path)`""
                    }
                    $process = Start-Process -FilePath $blender -ArgumentList $arguments -NoNewWindow -Wait -PassThru `
                        -RedirectStandardOutput $log -RedirectStandardError "$log.err"
                    if ($process.ExitCode -ne 0) { throw "Blender failed on $($asset.blender.script) (exit code $($process.ExitCode)), see $log" }
                    $status = if ($asset.kind -eq 'generated') { 'generated' } else { 'converted' }
                }
            }
            $missing = Get-Missing $asset
            if ($missing.Count -gt 0) { throw "missing after the fetch: $($missing -join ', ')" }
        } catch {
            $problem = "$_"
        }
    }
    if ($problem -and $status -ne 'absent') { $status = 'FAILED' }
    $rows += New-Object PSObject -Property ([ordered]@{
        Id = $asset.id; Kind = $asset.kind; License = $asset.license; Status = $status
        MB = [math]::Round((Get-FolderBytes $folder) / 1MB, 1); Problem = $problem })
}

$rows | Format-Table -AutoSize Id, Kind, License, Status, MB | Out-String -Width 200 | Write-Host
$failed = @($rows | Where-Object { $_.Status -eq 'FAILED' })
foreach ($row in $failed) { Write-Host "$($row.Id): $($row.Problem)" -ForegroundColor Red }
foreach ($row in @($rows | Where-Object { $_.Status -eq 'absent' })) { Write-Host "$($row.Id) absent: $($row.Problem)" -ForegroundColor Yellow }
Write-Host ('{0:N2} GB in {1} (budget {2} GB)' -f ((Get-FolderBytes $root) / 1GB), $root, $manifest.budgetGB)
if ($failed.Count -gt 0) { Write-Host "FAILED: $($failed.Count) of $($rows.Count) assets" -ForegroundColor Red; exit 1 }
Write-Host 'OK' -ForegroundColor Green
exit 0
