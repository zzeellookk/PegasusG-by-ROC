# Changelog

## Unreleased

- Split Brick CI output into a clean-install full package and a minimal update
  package, using the last device-working archive as the resource baseline.
- Added the Brick application entry, UI assets, 11 built-in tracks,
  modification-category list, installation notes and package validation.
- Kept FFmpeg, custom AArch64 cores, ZIP tools and a pre-enabled autostart hook
  out of the stable full package.
- Next targets: persistent ZIP cache and Brick rumble-output diagnosis.
- Do not promote files under `Brick/experimental/` into the stable launcher
  until they pass the device checklist in `Brick/STABLE_BASELINE.md`.

## 1.1.1-hotfix3-brick-stable-rollback — 2026-08-20

- Restored the known-good Brick launcher that uses firmware RetroArch cores.
- Retained the verified autostart handoff using `killall -9 MainUI`.
- Retained separate GBA, GBA hack and GBA vib scanning.
- Retained 1024x768 Brick display, input and disabled in-app power operations.
- Withdrew the combined AArch64 gpSP/ZIP/forced-rumble launcher from stable use.

## 1.1.1-hotfix4.1 experimental — 2026-08-20

- Built current gpSP as AArch64 with GLIBC 2.17 compatibility.
- Added a forced-rumble gpSP variant.
- Added safe extraction of one `.gba` or `.agb` member from a ZIP.
- Fixed BusyBox Chinese filenames displayed as question marks by extracting
  through the unique ASCII extension.
- Device result: normal cores launched, ZIP launch was slower because it was
  re-extracted every time, and forced-rumble gpSP froze at the first frame.
- This version is archived under `Brick/experimental/` and is not stable.

## 1.08 H700 — 2026

- Initial open-source H700 release by ROC.
