# PegasusG by ROC - H700 Build and Package

This directory builds the AArch64 application package for a stock H700 APPS
launcher. It uses the target firmware's SDL2, SDL2_image, SDL2_ttf and audio
libraries; it does not carry a second general-purpose runtime.

## Build

```powershell
.\H700\build_app.ps1 -Output Stage -Version 1.08 -Sysroot 'D:\path\to\sysroot'
.\H700\build_app.ps1 -Output Zip -Version 1.08 -Sysroot 'D:\path\to\sysroot'
```

The sysroot may instead be placed at `H700\sysroot\` (ignored by Git). The
default music source is `assets\music\builtin\`, which contains the selected
11-track set. Override it with `-MusicSource` when building with a complete,
separately licensed collection.

## Split Packages

After a staging build:

```powershell
.\H700\build_split_packages.ps1 -Version 1.08 -MainOnly
.\H700\build_split_packages.ps1 -Version 1.08 `
  -FullMusicSource 'D:\path\to\complete\Music'
```

The first command creates the small main package. The second additionally
creates the version-independent complete music supplement and expects 324 MP3
tracks. Archives are written under `H700\Downloads\`.

## Package Layout

```text
Roms/APPS/PegasusG by ROC.sh
Roms/APPS/PegasusG by ROC/
  pegasusg_by_roc
  launch.sh
  autostart_ctl.sh
  autostart_launch.sh
  tools/
  assets/cores/
  assets/filters/
  assets/recommended_controls/
  assets/splash/
  LICENSE.md
  NOTICE.md
  NOTICE.zh-CN.md
  THIRD_PARTY_NOTICES.md
  third_party/licenses/
```

## Content Roots

The launcher supports GBA, GBA hack and GBA vib collections on both card
mounts. Set `PEGASUSG_CONTENT_ROOTS` to override the default roots. ROMs and
commercial content are not included in this source repository.

## H700 Artwork

When the runtime identifies an H700 device, `use_mini_assets` selects the
copyright-bearing `mini_boxfront.png/.jpg` and `mini_logo.png/.jpg` files next
to the original media. Desktop and Android paths do not read these optimized
assets. Legacy `.bmp` thumbnail names are no longer part of the runtime
contract.

## Launcher Operations

```sh
"/mnt/mmc/Roms/APPS/PegasusG by ROC/autostart_ctl.sh" enable
"/mnt/mmc/Roms/APPS/PegasusG by ROC/autostart_ctl.sh" disable
"/mnt/mmc/Roms/APPS/PegasusG by ROC/autostart_ctl.sh" uninstall
```

The launcher stores state and logs under `/mnt/data/pegasusg-by-roc/`. See
`docs/PORTING_GUIDE.md` for adapting mount points, input mappings, power
services and splash targets to another H700-class machine.
