param(
  [ValidatePattern('^[0-9A-Za-z._-]+$')]
  [string]$Version = '1.08',
  [switch]$MainOnly,
  [string]$FullMusicSource = ''
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

$stageRoot = Join-Path $PSScriptRoot 'dist_app\release_stage'
$runtimeRoot = Join-Path $stageRoot 'Roms\APPS\PegasusG by ROC'
$musicRoot = Join-Path $runtimeRoot 'assets\music'
$downloads = Join-Path $PSScriptRoot 'Downloads'
$musicSuffix = -join @([char]0x97F3, [char]0x4E50, [char]0x8865, [char]0x5145, [char]0x5305)
$completeSuffix = -join @([char]0x5B8C, [char]0x6574)
$fireEmblemOption = (-join @(
  [char]0x706B, [char]0x7130, [char]0x7EB9, [char]0x7AE0, [char]0x20,
  [char]0x57C3, [char]0x529B, [char]0x683C, [char]0x4E4B, [char]0x67AA
)) + '.opt'
$dragonBallOption = (-join @(
  [char]0x53E3, [char]0x888B, [char]0x5996, [char]0x602A, [char]0x20,
  [char]0x9F99, [char]0x73E0, [char]0x5A, [char]0x20, [char]0x8D85,
  [char]0x6FC0, [char]0x6218
)) + '.opt'
$slimArchive = Join-Path $downloads "PegasusG by ROC ver$Version for H700.zip"
$musicArchive = Join-Path $downloads "PegasusG by ROC $completeSuffix$musicSuffix.zip"

if (-not (Test-Path -LiteralPath $runtimeRoot -PathType Container)) {
  throw "Missing staged runtime: $runtimeRoot"
}
if ((Get-Content -LiteralPath (Join-Path $runtimeRoot 'version.txt') -Raw).Trim() -ne $Version) {
  throw "Staged version does not match $Version"
}

$builtInNames = [System.Collections.Generic.HashSet[string]]::new(
  [System.StringComparer]::OrdinalIgnoreCase)
@(
  'WLF011.mp3',
  'WLF015.mp3',
  'WLF046.mp3',
  'WLF062.mp3',
  'WLF063.mp3',
  'WLF079.mp3',
  'WLF080.mp3',
  'WLF081.mp3',
  'WLF111.mp3',
  'WLF112.mp3',
  'WLF134.mp3'
) | ForEach-Object { [void]$builtInNames.Add($_) }

$stagedMusic = @(Get-ChildItem -LiteralPath $musicRoot -File -Filter '*.mp3' | Sort-Object Name)
$builtInMusic = @($stagedMusic | Where-Object { $builtInNames.Contains($_.Name) })

if ($builtInMusic.Count -ne $builtInNames.Count) {
  $missingBuiltIn = @($builtInNames | Where-Object { $_ -notin $stagedMusic.Name })
  throw "Expected 11 selected built-in tracks; missing: $($missingBuiltIn -join ', ')"
}

$completeMusic = $stagedMusic
if (-not $MainOnly -and $FullMusicSource) {
  if (-not (Test-Path -LiteralPath $FullMusicSource -PathType Container)) {
    throw "Missing complete music source: $FullMusicSource"
  }
  $completeMusic = @(Get-ChildItem -LiteralPath $FullMusicSource -File -Filter '*.mp3' | Sort-Object Name)
}
if (-not $MainOnly -and $completeMusic.Count -ne 324) {
  throw "Complete music package needs all 324 tracks. Stage them with -MusicSource or pass -FullMusicSource; found $($completeMusic.Count)."
}

New-Item -ItemType Directory -Force -Path $downloads | Out-Null

function Get-ArchivePath([System.IO.FileInfo]$File) {
  $relative = $File.FullName.Substring($stageRoot.Length).TrimStart('\', '/')
  return $relative.Replace('\', '/')
}

function New-FilteredArchive(
  [string]$ArchivePath,
  [System.IO.FileInfo[]]$Files
) {
  if (Test-Path -LiteralPath $ArchivePath) { Remove-Item -LiteralPath $ArchivePath -Force }
  $stream = [System.IO.File]::Open($ArchivePath, [System.IO.FileMode]::CreateNew)
  try {
    $archive = [System.IO.Compression.ZipArchive]::new(
      $stream, [System.IO.Compression.ZipArchiveMode]::Create, $false)
    try {
      foreach ($file in $Files) {
        [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
          $archive,
          $file.FullName,
          (Get-ArchivePath $file),
          [System.IO.Compression.CompressionLevel]::Optimal) | Out-Null
      }
    } finally {
      $archive.Dispose()
    }
  } finally {
    $stream.Dispose()
  }
}

function New-MusicArchive(
  [string]$ArchivePath,
  [System.IO.FileInfo[]]$Files,
  [string]$EntryPrefix
) {
  if (Test-Path -LiteralPath $ArchivePath) { Remove-Item -LiteralPath $ArchivePath -Force }
  $stream = [System.IO.File]::Open($ArchivePath, [System.IO.FileMode]::CreateNew)
  try {
    $archive = [System.IO.Compression.ZipArchive]::new(
      $stream, [System.IO.Compression.ZipArchiveMode]::Create, $false)
    try {
      foreach ($file in $Files) {
        [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
          $archive,
          $file.FullName,
          "$EntryPrefix$($file.Name)",
          [System.IO.Compression.CompressionLevel]::Optimal) | Out-Null
      }
    } finally {
      $archive.Dispose()
    }
  } finally {
    $stream.Dispose()
  }
}

$allStageFiles = @(Get-ChildItem -LiteralPath $stageRoot -Recurse -File)
$musicPrefix = 'Roms/APPS/PegasusG by ROC/assets/music/'
$slimFiles = @($allStageFiles | Where-Object {
  $relative = Get-ArchivePath $_
  -not $relative.StartsWith($musicPrefix, [System.StringComparison]::OrdinalIgnoreCase) -or
    $builtInNames.Contains($_.Name)
})

New-FilteredArchive -ArchivePath $slimArchive -Files $slimFiles
if (-not $MainOnly) {
  New-MusicArchive -ArchivePath $musicArchive -Files $completeMusic -EntryPrefix $musicPrefix
}

function Read-ArchiveEntries([string]$ArchivePath) {
  $archive = [System.IO.Compression.ZipFile]::OpenRead($ArchivePath)
  try {
    return @($archive.Entries | ForEach-Object { $_.FullName })
  } finally {
    $archive.Dispose()
  }
}

$slimEntries = @(Read-ArchiveEntries $slimArchive)
$musicEntries = @()
if (-not $MainOnly) {
  $musicEntries = @(Read-ArchiveEntries $musicArchive)
}
$slimTracks = @($slimEntries | Where-Object { $_ -like "$musicPrefix*.mp3" })
$supplementTracks = @($musicEntries | Where-Object { $_ -like "$musicPrefix*.mp3" })
$requiredEntries = @(
  'Roms/APPS/PegasusG by ROC.sh',
  'Roms/APPS/Imgs/PegasusG by ROC.png',
  'Roms/APPS/PegasusG by ROC/pegasusg_by_roc',
  'Roms/APPS/PegasusG by ROC/launch.sh',
  'Roms/APPS/PegasusG by ROC/tools/apply_filter.sh',
  'Roms/APPS/PegasusG by ROC/tools/apply_auto_cheats.sh',
  'Roms/APPS/PegasusG by ROC/tools/apply_game_overrides.sh',
  'Roms/APPS/PegasusG by ROC/tools/game_volume.sh',
  'Roms/APPS/PegasusG by ROC/tools/apply_recommended_controls.sh',
  'Roms/APPS/PegasusG by ROC/tools/apply_splash.sh',
  'Roms/APPS/PegasusG by ROC/version.txt',
  'Roms/APPS/PegasusG by ROC/LICENSE.md',
  'Roms/APPS/PegasusG by ROC/NOTICE.md',
  'Roms/APPS/PegasusG by ROC/NOTICE.zh-CN.md',
  'Roms/APPS/PegasusG by ROC/THIRD_PARTY_NOTICES.md',
  'Roms/APPS/PegasusG by ROC/third_party/licenses/mGBA-MPL-2.0.txt',
  'Roms/APPS/PegasusG by ROC/third_party/licenses/gpSP-GPL-2.0.txt',
  'Roms/APPS/PegasusG by ROC/third_party/licenses/VBA-M-License.txt',
  'Roms/APPS/PegasusG by ROC/third_party/licenses/VBA-Next-GPL-2.0.txt',
  'Roms/APPS/PegasusG by ROC/assets/cores/gpsp_rumble_libretro.so',
  'Roms/APPS/PegasusG by ROC/assets/cores/mgba_libretro.so',
  'Roms/APPS/PegasusG by ROC/assets/cores/gpsp_libretro.so',
  'Roms/APPS/PegasusG by ROC/assets/cores/vbam_libretro.so',
  'Roms/APPS/PegasusG by ROC/assets/cores/vba_next_libretro.so',
  'Roms/APPS/PegasusG by ROC/assets/ui/pegasus_g.png',
  'Roms/APPS/PegasusG by ROC/assets/cheats/gba-auto-cheats.zip',
  "Roms/APPS/PegasusG by ROC/assets/core_options/mGBA/$fireEmblemOption",
  "Roms/APPS/PegasusG by ROC/assets/core_options/mGBA/$dragonBallOption",
  'Roms/APPS/PegasusG by ROC/assets/filters/calibrated.glslp',
  'Roms/APPS/PegasusG by ROC/assets/filters/original.glslp',
  'Roms/APPS/PegasusG by ROC/assets/filters/shaders/image-adjustment.glsl',
  'Roms/APPS/PegasusG by ROC/assets/filters/shaders/nds-color.glsl',
  'Roms/APPS/PegasusG by ROC/assets/filters/shaders/retro-v2.glsl',
  'Roms/APPS/PegasusG by ROC/assets/recommended_controls/global.cfg',
  'Roms/APPS/PegasusG by ROC/assets/recommended_controls/core/mGBA.cfg',
  'Roms/APPS/PegasusG by ROC/assets/recommended_controls/core/gpSP.cfg',
  'Roms/APPS/PegasusG by ROC/assets/recommended_controls/core/VBA-M.cfg',
  'Roms/APPS/PegasusG by ROC/assets/recommended_controls/remaps/mGBA.rmp',
  'Roms/APPS/PegasusG by ROC/assets/recommended_controls/remaps/gpSP.rmp',
  'Roms/APPS/PegasusG by ROC/assets/recommended_controls/remaps/VBA-M.rmp',
  'Roms/APPS/PegasusG by ROC/assets/splash/bootlogo.bmp',
  'Roms/APPS/PegasusG by ROC/assets/splash/splash.png',
  'Roms/APPS/PegasusG by ROC/assets/splash/splash.jpg'
)
$missing = @($requiredEntries | Where-Object { $_ -notin $slimEntries })

if ($missing.Count -gt 0) { throw "Slim package missing: $($missing -join ', ')" }
if ($slimTracks.Count -ne 11) { throw "Slim package contains $($slimTracks.Count) tracks" }
if (-not $MainOnly) {
  if ($supplementTracks.Count -ne 324) {
    throw "Complete music package contains $($supplementTracks.Count) tracks"
  }
  $missingInSupplement = @($completeMusic.Name | Where-Object {
    "$musicPrefix$_" -notin $supplementTracks
  })
  if ($missingInSupplement.Count -gt 0) {
    throw "Complete music package is missing: $($missingInSupplement -join ', ')"
  }
  if (@($musicEntries | Where-Object { $_ -notlike "$musicPrefix*.mp3" }).Count -gt 0) {
    throw 'Music package contains files outside the expected music folder'
  }
}

$slimHash = (Get-FileHash -LiteralPath $slimArchive -Algorithm SHA256).Hash

[pscustomobject]@{
  Package = 'Slim release'
  Path = $slimArchive
  Tracks = $slimTracks.Count
  Bytes = (Get-Item -LiteralPath $slimArchive).Length
  SHA256 = $slimHash
}
if (-not $MainOnly) {
  $musicHash = (Get-FileHash -LiteralPath $musicArchive -Algorithm SHA256).Hash
  [pscustomobject]@{
    Package = 'Complete music supplement'
    Path = $musicArchive
    Tracks = $supplementTracks.Count
    Bytes = (Get-Item -LiteralPath $musicArchive).Length
    SHA256 = $musicHash
  }
}
