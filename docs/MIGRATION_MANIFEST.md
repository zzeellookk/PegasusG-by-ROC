# Migration Manifest

This repository is the extracted, self-contained development tree migrated
from the private ROC OS work area. The old work area remains a backup and is
not a build dependency.

## Included

- Portable frontend source in `src/`.
- Desktop and shell regression tests in `tests/`.
- H700 cross-build and launcher scripts in `H700/`.
- Runtime UI, core, cheat and built-in music assets in `assets/`.
- H700 splash assets in `H700/assets/splash/`.
- Architecture, baseline and transfer documents in `docs/`.
- Third-party license texts and provenance notes.

## Deliberately Excluded

- ROM images, commercial game media and metadata content packs.
- The complete music collection; only the selected 11-track built-in set is
  included. A separately obtained collection can be staged with the build
  scripts.
- Device-private SSH logs, MAC/IP captures, backups and deployment output.
- Firmware sysroots, toolchains, build caches, screenshots and historical ZIPs.
- Fonts whose redistribution permission could not be established.
- The original private configuration-package and ROCreader directories.

## Copyright Boundary

Extraction does not change ownership. Upstream Pegasus copyrights and license
terms remain in force. The H700/Linux port and project-specific changes remain
attributed to Blood_roc. Future ports should add their own attribution without
removing either existing notice.
