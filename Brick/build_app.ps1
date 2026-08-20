param(
  [string]$Version = '1.1.1-hotfix3-brick',
  [ValidateSet('Stage', 'Zip')][string]$Output = 'Stage',
  [Parameter(Mandatory = $true)][string]$Sysroot
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$rootWsl = (wsl wslpath -a $root).Trim()
$sysrootWsl = (wsl wslpath -a $Sysroot).Trim()

wsl bash -lc "cd '$rootWsl' && chmod +x ./Brick/build_app.sh && PEGASUSG_VERSION='$Version' PEGASUSG_OUTPUT='$Output' PEGASUSG_SYSROOT='$sysrootWsl' ./Brick/build_app.sh"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
