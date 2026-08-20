# Source package manifest

Prepared: 2026-08-20

Original repository:

```text
https://github.com/LPF970915/PegasusG-by-ROC.git
base commit: afeebff (v1.08)
```

This package overlays the current uncommitted Brick work on that base and adds
the missing Brick source/build/documentation layout. It intentionally contains
no `.git` directory, so it can be copied into an existing clone or used as the
contents of a newly created GitHub repository.

Included current changes:

- Brick compile-time frontend adaptations in `src/`.
- Brick collection scan, service and autostart tests.
- Stable hotfix3 Brick launcher and verified SIGKILL autostart handoff.
- Reproducible Brick frontend build scripts.
- Experimental AArch64 gpSP patch and launcher, isolated from stable files.
- GitHub Actions test workflow, changelog and development workflow.

Excluded:

- `.git/`, `build/`, local sysroots and cross-toolchain files.
- Compiler temporary `cc*.o` and `cc*.s` files.
- Device logs, configuration state, ROMs, BIOS and save data.
- Generated install archives, including the former tracked H700 v1.08 ZIP.
- Compiled experimental Brick frontend and gpSP core binaries.

The original small runtime assets under `assets/`, including the H700-managed
core binaries and 11 built-in music tracks, are retained together with the
existing third-party notices so the original H700 packaging flow remains
usable.
