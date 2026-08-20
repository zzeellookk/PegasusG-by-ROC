# Brick AArch64 libretro cores

Downloaded from the official Libretro Linux AArch64 buildbot on 2026-08-20:

https://buildbot.libretro.com/nightly/linux/aarch64/latest/

The packaged files are ELF64 AArch64 shared objects. They are loaded directly
by `/mnt/SDCARD/RetroArch/ra64.trimui` with the RetroArch `-L` option and do not
overwrite firmware cores.

SHA-256:

```
105b4760f1404486a8eb694a9d9638d67b97f78640a7428526e070c837905061  gpsp_libretro.so
abde7a0764f08fa0cc2c7d3d9a29b9d1245a9f3b7df0e7a594b74df642ee53c6  mgba_libretro.so
92e6204c1f948ffc09b4552fa1af462450e2161357f93c3c5b97748762955e56  vba_next_libretro.so
ba836c48a7c8e4113b903a4b290c739351b94ec78739e11b5b39d9e09f2d3e13  vbam_libretro.so
```

These binaries still require real-device validation on TrimUI Brick before a
stable release is published.
