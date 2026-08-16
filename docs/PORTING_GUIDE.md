# PegasusG by ROC Porting Guide

This guide describes the boundaries a maintainer should adapt for a new Linux
handheld. Keep game-library behavior and the upstream attribution unchanged;
put machine differences behind the H700 launcher and service layer.

## Build Inputs

1. A host compiler with SDL2, SDL2_image, SDL2_ttf and ALSA development files
   for desktop tests.
2. An AArch64 cross compiler and a sysroot copied from the target firmware.
3. A writable staging directory. `H700/sysroot/` is intentionally ignored by
   Git because firmware sysroots are device-specific and redistributability is
   uncertain.

## Files to Adapt

- `src/h700_services.*`: volume, brightness, hall sensor, suspend and power
  behavior. Replace sysfs paths and command fallbacks for the new device.
- `H700/launcher/launch.sh`: card mount points, RetroArch executable, core
  search paths and exit/restart supervision.
- `H700/launcher/autostart_*.sh`: vendor startup hook and rollback marker.
- `H700/launcher/apply_splash.sh`: boot partition and vendor transition-image
  paths. The script backs up every target before replacing it and restores the
  original set when disabled.
- `H700/input_map/` and the SDL/evdev mapping in `src/gba_frontend.cpp`:
  buttons, menu key, hinge wake and long-press behavior.
- `src/layout.*` and renderer constants: only if the panel resolution or
  rotation differs from the 720x480 logical layout.

## Content and Assets

The frontend reads Pegasus metadata from configurable roots. Set
`PEGASUSG_CONTENT_ROOTS` to a colon-separated list on Linux. Keep per-device
optimized artwork opt-in (`use_mini_assets`) so desktop and Android clients do
not accidentally read H700-only `mini_boxfront` or `mini_logo` assets.

The project no longer depends on legacy `.bmp` cover fallbacks. H700 optimized
assets are the current copyright-bearing `mini_boxfront.png/.jpg` and
`mini_logo.png/.jpg` files supplied beside the original media.

## Package Flow

1. Build and run `make test` on the host.
2. Cross-build with `H700/build_app.ps1 -Sysroot ...`.
3. Inspect `H700/dist_app/release_stage/` before making a ZIP.
4. Run `build_split_packages.ps1 -MainOnly` for the small release package.
5. Use a separately licensed complete music directory for the optional
   324-track supplement.
6. Test installation and rollback on a disposable card before publishing.

## Porting Checklist

- [ ] Display opens at the correct orientation and logical size.
- [ ] A/B/X/Y, d-pad, L1/L2, R1/R2, Menu, Select and Start are mapped.
- [ ] Volume and brightness changes survive frontend/game transitions.
- [ ] Lid suspend resumes the previous library selection.
- [ ] Both card roots scan correctly and saves stay with the ROM card.
- [ ] Core selection reads metadata first and uses the category fallback.
- [ ] Splash enable/disable restores the exact pre-existing vendor assets.
- [ ] Original Pegasus and Blood_roc notices remain visible and in the package.
