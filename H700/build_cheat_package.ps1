param()

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

$workspaceRoot = Split-Path $PSScriptRoot -Parent
$downloads = Join-Path $PSScriptRoot 'Downloads'
$sourceRoot = Join-Path $PSScriptRoot 'cheat_importer'
$stageRoot = Join-Path $PSScriptRoot ('dist_cheat_package\stage-{0}-{1}' -f ([DateTime]::UtcNow.ToString('yyyyMMddHHmmss')), $PID)

$sourceFileName = 'gba' + (-join @(
  [char]0x91D1, [char]0x624B, [char]0x6307,
  [char]0x5927, [char]0x5168)) + '.zip'
$appSuffix = -join @(
  [char]0x5BFC, [char]0x5165, 'G', 'B', 'A',
  [char]0x91D1, [char]0x624B, [char]0x6307,
  [char]0x5927, [char]0x5168)
$appName = "PegasusG by ROC $appSuffix"

$sourceMatches = @(Get-ChildItem -LiteralPath $workspaceRoot -Recurse -File -Filter $sourceFileName |
  Where-Object { $_.FullName -notlike "$PSScriptRoot\*" })
if ($sourceMatches.Count -ne 1) {
  throw "Expected one source archive named $sourceFileName, found $($sourceMatches.Count)"
}
$payloadSource = $sourceMatches[0].FullName

function Test-SafeEntry([string]$Name) {
  $normalized = $Name.Replace('\', '/')
  if ($normalized.StartsWith('/') -or $normalized -match '^[A-Za-z]:') { return $false }
  foreach ($part in $normalized.Split('/')) {
    if ($part -eq '..') { return $false }
  }
  return $true
}

function Read-ZipEntries([string]$ArchivePath) {
  $archive = [System.IO.Compression.ZipFile]::OpenRead($ArchivePath)
  try {
    return @($archive.Entries | ForEach-Object { $_.FullName })
  } finally {
    $archive.Dispose()
  }
}

function Get-ArchivePath([System.IO.FileInfo]$File, [string]$Root) {
  $relative = $File.FullName.Substring($Root.Length).TrimStart('\', '/')
  return $relative.Replace('\', '/')
}

function New-ZipArchive([string]$ArchivePath, [string]$Root) {
  if (Test-Path -LiteralPath $ArchivePath) {
    Remove-Item -LiteralPath $ArchivePath -Force
  }

  $stream = [System.IO.File]::Open($ArchivePath, [System.IO.FileMode]::CreateNew)
  try {
    $archive = [System.IO.Compression.ZipArchive]::new(
      $stream, [System.IO.Compression.ZipArchiveMode]::Create, $false)
    try {
      foreach ($file in (Get-ChildItem -LiteralPath $Root -Recurse -File)) {
        [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
          $archive,
          $file.FullName,
          (Get-ArchivePath $file $Root),
          [System.IO.Compression.CompressionLevel]::Optimal) | Out-Null
      }
    } finally {
      $archive.Dispose()
    }
  } finally {
    $stream.Dispose()
  }
}

$payloadEntries = @(Read-ZipEntries $payloadSource)
$unsafeEntries = @($payloadEntries | Where-Object { -not (Test-SafeEntry $_) })
$cheatEntries = @($payloadEntries | Where-Object { $_ -like '*.cht' })
if ($unsafeEntries.Count -gt 0) {
  throw "Source archive contains unsafe paths: $($unsafeEntries -join ', ')"
}
if ($cheatEntries.Count -ne 2801) {
  throw "Expected 2801 .cht files, found $($cheatEntries.Count)"
}

$appsRoot = Join-Path $stageRoot 'Roms\APPS'
$appRoot = Join-Path $appsRoot $appName
New-Item -ItemType Directory -Force -Path $appRoot, $downloads | Out-Null
Copy-Item -LiteralPath (Join-Path $sourceRoot 'entry.sh') -Destination (Join-Path $appsRoot ($appName + '.sh'))
Copy-Item -LiteralPath (Join-Path $sourceRoot 'install.sh') -Destination (Join-Path $appRoot 'install.sh')
Copy-Item -LiteralPath $payloadSource -Destination (Join-Path $appRoot 'gba-cheats.zip')

$archivePath = Join-Path $downloads ($appName + '.zip')
New-ZipArchive -ArchivePath $archivePath -Root $stageRoot

$entries = @(Read-ZipEntries $archivePath)
$prefix = 'Roms/APPS/' + $appName
$required = @(
  ($prefix + '.sh'),
  ($prefix + '/install.sh'),
  ($prefix + '/gba-cheats.zip')
)
$missing = @($required | Where-Object { $_ -notin $entries })
if ($missing.Count -gt 0) {
  throw "Package missing entries: $($missing -join ', ')"
}
if (@($entries | Where-Object { $_ -notlike "$prefix*" }).Count -gt 0) {
  throw "Package contains files outside $prefix"
}

$item = Get-Item -LiteralPath $archivePath
[pscustomobject]@{
  Package = $appName
  Path = $item.FullName
  Entries = $entries.Count
  Cheats = $cheatEntries.Count
  Bytes = $item.Length
  PayloadSHA256 = (Get-FileHash -LiteralPath $payloadSource -Algorithm SHA256).Hash
  SHA256 = (Get-FileHash -LiteralPath $archivePath -Algorithm SHA256).Hash
}
