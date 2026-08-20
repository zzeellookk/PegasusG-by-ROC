# Experimental AArch64 gpSP build

Upstream: `https://github.com/libretro/gpsp`

Pinned commit:

```text
1e0fe9f734dcb2e3bea037f483045b30737c37c8
```

The upstream core already supports ARMv8/AArch64 and the libretro rumble
interface. The local patch forces rumble detection for homebrew and ROM hacks.
That forced variant currently freezes at the first frame on the Brick test
device, so it must remain experimental.

Build from a disposable checkout of the pinned upstream revision:

```sh
GPSP_SRC=/path/to/gpsp \
PEGASUSG_SYSROOT=/path/to/aarch64-sysroot \
./Brick/experimental/gpsp-aarch64/build.sh
```

The generated cores are written below `Brick/build/gpsp-aarch64/` and are
ignored by Git. Do not commit the compiled `.so` files; attach tested binaries
to a GitHub Release with checksums and corresponding source information.
