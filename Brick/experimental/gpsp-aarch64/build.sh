#!/bin/bash
set -euo pipefail

SELF_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "$SELF_DIR/../../.." && pwd)"
SOURCE="${GPSP_SRC:?set GPSP_SRC to a disposable gpSP checkout}"
SYSROOT="${PEGASUSG_SYSROOT:?set PEGASUSG_SYSROOT to the AArch64 sysroot}"
EXPECTED_COMMIT="1e0fe9f734dcb2e3bea037f483045b30737c37c8"
OUTPUT="$REPO_ROOT/Brick/build/gpsp-aarch64"
NORMAL="$OUTPUT/normal-src"
RUMBLE="$OUTPUT/rumble-src"

actual="$(git -C "$SOURCE" rev-parse HEAD)"
[ "$actual" = "$EXPECTED_COMMIT" ] || {
  echo "gpSP commit mismatch: expected $EXPECTED_COMMIT, got $actual" >&2
  exit 2
}

mkdir -p "$OUTPUT"
rm -rf "$NORMAL" "$RUMBLE"
cp -a "$SOURCE" "$NORMAL"
cp -a "$SOURCE" "$RUMBLE"

CC="${CROSS_CC:-aarch64-linux-gnu-gcc-9}"
CXX="${CROSS_CXX:-aarch64-linux-gnu-g++-9}"
AR="${CROSS_AR:-aarch64-linux-gnu-ar}"
jobs="${JOBS:-4}"

build_core() {
  source_dir="$1"
  output_name="$2"
  extra_define="$3"
  make -C "$source_dir" clean
  make -C "$source_dir" -j"$jobs" platform=arm64 \
    CC="$CC --sysroot=$SYSROOT" \
    CXX="$CXX --sysroot=$SYSROOT" \
    AR="$AR" LDFLAGS=-static-libgcc CODE_DEFINES="$extra_define"
  cp "$source_dir/gpsp_libretro.so" "$OUTPUT/$output_name"
}

build_core "$NORMAL" gpsp_libretro.so ""
patch -d "$RUMBLE" -p1 < "$SELF_DIR/patches/0001-pegasusg-force-rumble.patch"
build_core "$RUMBLE" gpsp_rumble_libretro.so -DPEGASUSG_FORCE_RUMBLE

file "$OUTPUT/"*.so
sha256sum "$OUTPUT/"*.so
