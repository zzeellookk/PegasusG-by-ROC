param(
  [string]$SourceRoot = '',
  [string]$OutputPath = '',
  [int]$ExpectedCount = 1203
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if (-not $SourceRoot) {
  $candidates = @(Get-ChildItem -LiteralPath $repoRoot -Directory | ForEach-Object {
    Get-ChildItem -LiteralPath $_.FullName -Directory -Filter 'PG_*_Roms' -ErrorAction SilentlyContinue
  } | ForEach-Object {
    Join-Path $_.FullName 'Roms'
  } | Where-Object {
    Test-Path -LiteralPath $_ -PathType Container
  })
  if ($candidates.Count -ne 1) {
    throw "Expected one PG_*_Roms/Roms source, found $($candidates.Count)"
  }
  $SourceRoot = $candidates[0]
}
$SourceRoot = (Resolve-Path -LiteralPath $SourceRoot).Path
if (-not $OutputPath) {
  $suffix = -join @(
    [char]0x5C01, [char]0x9762, [char]0x4F18, [char]0x5316,
    [char]0x8865, [char]0x5145, [char]0x5305)
  $OutputPath = Join-Path $PSScriptRoot "Downloads\PegasusG by ROC 2026 GBA$suffix.zip"
}
$outputDirectory = Split-Path -Parent $OutputPath
New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null

$files = @(Get-ChildItem -LiteralPath $SourceRoot -Recurse -File -Filter '*.h700.bmp' |
  Sort-Object FullName)
if ($files.Count -ne $ExpectedCount) {
  throw "Expected $ExpectedCount H700 thumbnails, found $($files.Count)"
}

function Get-EntryPath([System.IO.FileInfo]$File) {
  $relative = $File.FullName.Substring($SourceRoot.Length).TrimStart('\', '/')
  return 'Roms/' + $relative.Replace('\', '/')
}

if (Test-Path -LiteralPath $OutputPath) {
  Remove-Item -LiteralPath $OutputPath -Force
}
$stream = [System.IO.File]::Open($OutputPath, [System.IO.FileMode]::CreateNew)
try {
  $archive = [System.IO.Compression.ZipArchive]::new(
    $stream, [System.IO.Compression.ZipArchiveMode]::Create, $false)
  try {
    foreach ($file in $files) {
      [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
        $archive,
        $file.FullName,
        (Get-EntryPath $file),
        [System.IO.Compression.CompressionLevel]::Optimal) | Out-Null
    }
  } finally {
    $archive.Dispose()
  }
} finally {
  $stream.Dispose()
}

$archive = [System.IO.Compression.ZipFile]::OpenRead($OutputPath)
try {
  $entries = @($archive.Entries | ForEach-Object { $_.FullName })
} finally {
  $archive.Dispose()
}
if ($entries.Count -ne $ExpectedCount) {
  throw "Archive contains $($entries.Count) entries"
}
$invalid = @($entries | Where-Object {
  -not $_.StartsWith('Roms/', [System.StringComparison]::Ordinal) -or
  -not $_.EndsWith('.h700.bmp', [System.StringComparison]::OrdinalIgnoreCase)
})
if ($invalid.Count -gt 0) {
  throw "Archive contains invalid paths: $($invalid -join ', ')"
}

$collections = $entries | ForEach-Object { ($_ -split '/')[1] } |
  Group-Object | Sort-Object Name | ForEach-Object {
    [pscustomobject]@{ Collection = $_.Name; Thumbnails = $_.Count }
  }
$hash = (Get-FileHash -LiteralPath $OutputPath -Algorithm SHA256).Hash

$collections
[pscustomobject]@{
  Package = $OutputPath
  Entries = $entries.Count
  Bytes = (Get-Item -LiteralPath $OutputPath).Length
  SHA256 = $hash
}
