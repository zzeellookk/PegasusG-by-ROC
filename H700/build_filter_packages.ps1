param()

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

$sourceRoot = Join-Path $PSScriptRoot 'filter_injectors'
$downloads = Join-Path $PSScriptRoot 'Downloads'
$stageRoot = Join-Path $PSScriptRoot ('dist_filter_packages\stage-{0}-{1}' -f ([DateTime]::UtcNow.ToString('yyyyMMddHHmmss')), $PID)

$calibratedSuffix = -join @(
  [char]0x4E00, [char]0x952E, [char]0x6CE8, [char]0x5165,
  [char]0x6821, [char]0x8272, [char]0x6EE4, [char]0x955C)
$originalSuffix = -join @(
  [char]0x4E00, [char]0x952E, [char]0x6CE8, [char]0x5165,
  [char]0x539F, [char]0x8272, [char]0x6EE4, [char]0x955C)

$packages = @(
  [pscustomobject]@{
    Variant = 'calibrated'
    AppName = "PegasusG by ROC $calibratedSuffix"
  },
  [pscustomobject]@{
    Variant = 'original'
    AppName = "PegasusG by ROC $originalSuffix"
  }
)

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

function Read-ZipEntries([string]$ArchivePath) {
  $archive = [System.IO.Compression.ZipFile]::OpenRead($ArchivePath)
  try {
    return @($archive.Entries | ForEach-Object { $_.FullName })
  } finally {
    $archive.Dispose()
  }
}

New-Item -ItemType Directory -Force -Path $downloads, $stageRoot | Out-Null

$results = foreach ($package in $packages) {
  $packageRoot = Join-Path $stageRoot $package.Variant
  $appsRoot = Join-Path $packageRoot 'Roms\APPS'
  $appRoot = Join-Path $appsRoot $package.AppName
  $shaderRoot = Join-Path $appRoot 'shaders'
  $variantRoot = Join-Path $sourceRoot $package.Variant

  New-Item -ItemType Directory -Force -Path $shaderRoot | Out-Null
  Copy-Item -LiteralPath (Join-Path $sourceRoot 'common\entry.sh') -Destination (Join-Path $appsRoot ($package.AppName + '.sh'))
  Copy-Item -LiteralPath (Join-Path $sourceRoot 'common\install.sh') -Destination (Join-Path $appRoot 'install.sh')
  Copy-Item -LiteralPath (Join-Path $variantRoot 'variant.txt') -Destination (Join-Path $appRoot 'variant.txt')
  Copy-Item -LiteralPath (Join-Path $variantRoot 'preset.glslp') -Destination (Join-Path $appRoot 'preset.glslp')
  Copy-Item -Path (Join-Path $sourceRoot 'common\shaders\*.glsl') -Destination $shaderRoot

  $archivePath = Join-Path $downloads ($package.AppName + '.zip')
  New-ZipArchive -ArchivePath $archivePath -Root $packageRoot

  $entries = @(Read-ZipEntries $archivePath)
  $prefix = 'Roms/APPS/' + $package.AppName
  $required = @(
    ($prefix + '.sh'),
    ($prefix + '/install.sh'),
    ($prefix + '/variant.txt'),
    ($prefix + '/preset.glslp'),
    ($prefix + '/shaders/image-adjustment.glsl'),
    ($prefix + '/shaders/nds-color.glsl'),
    ($prefix + '/shaders/retro-v2.glsl')
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
    Package = $package.AppName
    Path = $item.FullName
    Entries = $entries.Count
    Bytes = $item.Length
    SHA256 = (Get-FileHash -LiteralPath $archivePath -Algorithm SHA256).Hash
  }
}

$results
