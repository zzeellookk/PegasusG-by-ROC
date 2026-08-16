# PegasusG by ROC

PegasusG by ROC is a downstream Linux SDL2 frontend for GBA-focused Pegasus
content on H700 handhelds. It is a community port and adaptation project; it
is **not** an official Pegasus Frontend release.

The repository is intended to be cloned as a complete development project.
Machine-specific code is kept under `H700/`, application code under `src/`,
and portable tests under `tests/`.

## Attribution

- The original Pegasus Frontend project and its contributors retain their
  copyright and licensing rights. See `LICENSE.md`, `NOTICE.md`, and
  `NOTICE.zh-CN.md`.
- The H700 SDL2 port, PegasusG by ROC integration, launcher scripts, and
  project-specific UI work are Copyright (c) 2026 Blood_roc.
- Contact: QQ `825826146`.

Downstream ports must keep these notices, the in-app `天马G ROC移植`
attribution, and the relevant third-party notices. Mark significant changes
clearly and do not present a port as an official Pegasus release.

## Repository Layout

```text
src/                       Portable frontend implementation
tests/                     Desktop and shell regression tests
assets/                    Small runtime assets and 11 built-in music tracks
H700/                      Cross-build, launcher, filter, input and packaging tools
docs/                      Architecture, baseline and migration documentation
third_party/licenses/      Licenses copied for bundled core binaries
```

ROMs, commercial game media, the full music collection, device dumps and
machine-private logs are intentionally not part of this repository.

## Desktop Build

On a Linux/WSL environment with SDL2, SDL2_image, SDL2_ttf and ALSA development
packages:

```sh
make
make test
```

The Windows helper `run-dev.ps1` starts `build/pegasusg_by_roc.exe` after the
desktop build. A system font is used by default; a font can be supplied at
build/package time with `PEGASUSG_FONT_SOURCE` or `-FontSource` when its license
permits redistribution.

## H700 Cross Build

The H700 build needs an AArch64 compiler and a sysroot from the target firmware.
The sysroot is deliberately not committed. Put it at `H700/sysroot/` or pass
one explicitly:

```powershell
.\H700\build_app.ps1 `
  -Version 1.08 `
  -Output Stage `
  -Sysroot 'D:\path\to\h700\sysroot'
```

The default package includes the 11 tracks in `assets/music/builtin/`. To stage
a separately obtained complete music collection, pass `-MusicSource` with the
directory containing the MP3 files. `build_split_packages.ps1 -MainOnly`
creates only the small main package; a complete 324-track supplement can be
created with `-FullMusicSource`.

```powershell
.\H700\build_split_packages.ps1 -Version 1.08 -MainOnly
.\H700\build_split_packages.ps1 -Version 1.08 `
  -FullMusicSource 'D:\path\to\complete\Music'
```

The resulting archives are written to `H700/Downloads/`, which is ignored by
Git except for formal versioned frontend packages and the directory README.
Each formal package is also published through the matching GitHub Release.
See `docs/PORTING_GUIDE.md` before adapting the launcher to another machine.

## Runtime Content

The frontend scans Pegasus metadata and ROM directories on both card roots.
The H700 launcher, RetroArch core replacement, filters, recommended controls,
cheat import and splash hooks are packaged as separate scripts under
`H700/launcher/` and related directories. No ROM image is included.

## License

The frontend code is distributed under GPLv3 with the Pegasus Frontend
additional terms reproduced in `LICENSE.md`. Bundled cores, music, logos,
filters and cheat data have their own provenance and licensing conditions;
read `THIRD_PARTY_NOTICES.md` before redistributing a package.
