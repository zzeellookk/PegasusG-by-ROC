# Brick AArch64 libretro cores

Downloaded from the official Libretro Linux AArch64 buildbot on 2026-08-20:

https://buildbot.libretro.com/nightly/linux/aarch64/latest/

The packaged files are ELF64 AArch64 shared objects. They are loaded directly
by `/mnt/SDCARD/RetroArch/ra64.trimui` with the RetroArch `-L` option and do not
overwrite firmware cores.

`gpsp_rumble_libretro.so` is built by GitHub Actions from upstream gpSP revision
`5b6e751f4abf368509146cd143c949c1946ac1ae`. This is the source revision that
contains the newer GPIO, EZ-Flash and Game Boy Player rumble emulation inspected
in the Android reference core. The Android binary itself is not redistributed
or loaded on Brick.

SHA-256:

```
105b4760f1404486a8eb694a9d9638d67b97f78640a7428526e070c837905061  gpsp_libretro.so
abde7a0764f08fa0cc2c7d3d9a29b9d1245a9f3b7df0e7a594b74df642ee53c6  mgba_libretro.so
92e6204c1f948ffc09b4552fa1af462450e2161357f93c3c5b97748762955e56  vba_next_libretro.so
ba836c48a7c8e4113b903a4b290c739351b94ec78739e11b5b39d9e09f2d3e13  vbam_libretro.so
```

The SHA-256 of `gpsp_rumble_libretro.so` is emitted by CI because it is built
from the pinned source revision during every package run.

These binaries still require real-device validation on TrimUI Brick before a
stable release is published.
