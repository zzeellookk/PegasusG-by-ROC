param(
  [Parameter(Mandatory = $true)]
  [string]$Root,
  [int]$CoverSize = 192,
  [int]$LogoWidth = 256,
  [int]$LogoHeight = 96,
  [switch]$Force
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$Root = (Resolve-Path -LiteralPath $Root).Path
$sourceNames = [System.Collections.Generic.HashSet[string]]::new(
  [System.StringComparer]::OrdinalIgnoreCase)
@('boxfront.png', 'cover.png', 'boxfront.jpg', 'cover.jpg', 'logo.png', 'logo.jpg') |
  ForEach-Object { [void]$sourceNames.Add($_) }

$sources = @(Get-ChildItem -LiteralPath $Root -Recurse -File | Where-Object {
  $sourceNames.Contains($_.Name)
})
$created = 0
$skipped = 0

foreach ($source in $sources) {
  $isLogo = $source.BaseName.Equals('logo', [System.StringComparison]::OrdinalIgnoreCase)
  $width = if ($isLogo) { $LogoWidth } else { $CoverSize }
  $height = if ($isLogo) { $LogoHeight } else { $CoverSize }
  $output = Join-Path $source.DirectoryName ($source.BaseName + '.h700.bmp')
  if (-not $Force -and (Test-Path -LiteralPath $output)) {
    $existing = Get-Item -LiteralPath $output
    if ($existing.LastWriteTimeUtc -ge $source.LastWriteTimeUtc) {
      $skipped++
      continue
    }
  }

  $image = [System.Drawing.Image]::FromFile($source.FullName)
  try {
    $scale = [Math]::Min($width / [double]$image.Width, $height / [double]$image.Height)
    $drawWidth = [Math]::Max(1, [int][Math]::Round($image.Width * $scale))
    $drawHeight = [Math]::Max(1, [int][Math]::Round($image.Height * $scale))
    $bitmap = [System.Drawing.Bitmap]::new(
      $width, $height, [System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
    try {
      $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
      try {
        $graphics.Clear([System.Drawing.Color]::Black)
        $graphics.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
        $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
        $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
        $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
        $x = [int](($width - $drawWidth) / 2)
        $y = [int](($height - $drawHeight) / 2)
        $graphics.DrawImage($image, $x, $y, $drawWidth, $drawHeight)
      } finally {
        $graphics.Dispose()
      }
      $temporary = $output + '.tmp'
      $bitmap.Save($temporary, [System.Drawing.Imaging.ImageFormat]::Bmp)
      Move-Item -LiteralPath $temporary -Destination $output -Force
    } finally {
      $bitmap.Dispose()
    }
  } finally {
    $image.Dispose()
  }
  $created++
}

$coverCount = @($sources | Where-Object {
  -not $_.BaseName.Equals('logo', [System.StringComparison]::OrdinalIgnoreCase)
}).Count
$logoCount = $sources.Count - $coverCount
Write-Host "cover_size=$CoverSize logo_size=${LogoWidth}x${LogoHeight} covers=$coverCount logos=$logoCount sources=$($sources.Count) created=$created skipped=$skipped"
