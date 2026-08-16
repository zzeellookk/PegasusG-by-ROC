param(
  [ValidateRange(320, 7680)]
  [int]$Width = 1280,

  [ValidateRange(240, 4320)]
  [int]$Height = 800,

  [ValidateRange(0, 8)]
  [int]$Module = 1,

  [string]$Msys2Bin = ''
)

$executable = Join-Path $PSScriptRoot 'build\pegasusg_by_roc.exe'

if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
  throw "PegasusG by ROC is not built: $executable"
}

$candidates = @(
  $Msys2Bin,
  $env:PEGASUSG_MSYS2_BIN,
  'C:\msys64\ucrt64\bin',
  'D:\Program Files\MSYS2\ucrt64\bin'
) | Where-Object { $_ -and (Test-Path -LiteralPath $_ -PathType Container) }
if ($candidates.Count -gt 0) {
  $env:PATH = "$($candidates[0]);$env:PATH"
}
& $executable --width $Width --height $Height --module $Module
