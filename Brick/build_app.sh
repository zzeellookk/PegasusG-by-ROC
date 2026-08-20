#!/bin/bash
set -euo pipefail

SELF_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "$SELF_DIR/.." && pwd)"

VERSION="${PEGASUSG_VERSION:-1.1.1-hotfix3-brick}"
OUTPUT="${PEGASUSG_OUTPUT:-Stage}"
SYSROOT="${PEGASUSG_SYSROOT:-$SELF_DIR/sysroot}"
BUILD_DIR="$SELF_DIR/build"
STAGE_ROOT="$SELF_DIR/dist/stage"
APP_DIR="$STAGE_ROOT/Apps/PegasusG"
ARCHIVE="$SELF_DIR/dist/PegasusG-Brick-${VERSION}.zip"

case "$OUTPUT" in Stage|Zip) ;; *) echo "OUTPUT must be Stage or Zip" >&2; exit 2 ;; esac
[ -d "$SYSROOT/usr/include" ] || { echo "invalid Brick sysroot: $SYSROOT" >&2; exit 3; }

if [ -n "${CROSS_CXX:-}" ]; then
  CXX_CMD="$CROSS_CXX"
elif command -v aarch64-linux-gnu-g++-11 >/dev/null 2>&1; then
  CXX_CMD="aarch64-linux-gnu-g++-11"
elif command -v aarch64-linux-gnu-g++-9 >/dev/null 2>&1; then
  CXX_CMD="aarch64-linux-gnu-g++-9"
else
  CXX_CMD="aarch64-linux-gnu-g++"
fi

PKG_CMD="${CROSS_PKG_CONFIG:-pkg-config}"
PKG_DIRS=""
for dir in \
  "$SYSROOT/usr/lib/aarch64-linux-gnu/pkgconfig" \
  "$SYSROOT/lib/aarch64-linux-gnu/pkgconfig" \
  "$SYSROOT/usr/lib/pkgconfig" \
  "$SYSROOT/lib/pkgconfig"; do
  [ -d "$dir" ] && PKG_DIRS="${PKG_DIRS}${PKG_DIRS:+:}$dir"
done
export PKG_CONFIG_SYSROOT_DIR="$SYSROOT"
export PKG_CONFIG_LIBDIR="$PKG_DIRS"
export PKG_CONFIG_PATH=""

pkg_cflags="$($PKG_CMD --cflags sdl2 SDL2_image SDL2_ttf 2>/dev/null || true)"
pkg_libs="$($PKG_CMD --libs sdl2 SDL2_image SDL2_ttf alsa 2>/dev/null || true)"
[ -n "$pkg_cflags" ] || pkg_cflags="-I$SYSROOT/usr/include/SDL2 -I$SYSROOT/usr/include -D_REENTRANT"
[ -n "$pkg_libs" ] || pkg_libs="-L$SYSROOT/usr/lib -L$SYSROOT/usr/lib/aarch64-linux-gnu -L$SYSROOT/lib/aarch64-linux-gnu -lSDL2_image -lSDL2_ttf -lSDL2 -lasound"

mkdir -p "$BUILD_DIR" "$APP_DIR"
"$CXX_CMD" --sysroot="$SYSROOT" -DPEGASUSG_BRICK \
  -std=c++17 -O2 -DNDEBUG -Wall -Wextra -Wpedantic -pthread \
  -I"$REPO_ROOT/src" $pkg_cflags \
  "$REPO_ROOT/src/main.cpp" \
  "$REPO_ROOT/src/gba_frontend.cpp" \
  "$REPO_ROOT/src/gba_preferences.cpp" \
  "$REPO_ROOT/src/gba_state.cpp" \
  "$REPO_ROOT/src/gba_ui_state.cpp" \
  "$REPO_ROOT/src/h700_services.cpp" \
  "$REPO_ROOT/src/library_scanner.cpp" \
  "$REPO_ROOT/src/optimized_image_path.cpp" \
  "$REPO_ROOT/src/pegasus_metadata.cpp" \
  "$REPO_ROOT/src/video_preview.cpp" \
  -o "$BUILD_DIR/pegasusg_by_roc" $pkg_libs -pthread \
  -Wl,-rpath-link,"$SYSROOT/usr/lib/aarch64-linux-gnu" \
  -Wl,-rpath-link,"$SYSROOT/lib/aarch64-linux-gnu" \
  -Wl,-rpath-link,"$SYSROOT/usr/lib" \
  -Wl,-rpath-link,"$SYSROOT/lib" \
  -Wl,--allow-shlib-undefined

cp "$BUILD_DIR/pegasusg_by_roc" "$APP_DIR/pegasusg_by_roc"
cp "$SELF_DIR/launcher/launch.sh" "$APP_DIR/launch.sh"
cp "$SELF_DIR/launcher/autostart_ctl.sh" "$APP_DIR/autostart_ctl.sh"
cp "$SELF_DIR/launcher/autostart_launch.sh" "$APP_DIR/autostart_launch.sh"
printf '%s\n' "$VERSION" > "$APP_DIR/version.txt"
chmod 755 "$APP_DIR/pegasusg_by_roc" "$APP_DIR/"*.sh

if [ "$OUTPUT" = "Zip" ]; then
  python3 - "$STAGE_ROOT" "$ARCHIVE" <<'PY'
import pathlib
import sys
import zipfile

stage = pathlib.Path(sys.argv[1])
archive = pathlib.Path(sys.argv[2])
archive.parent.mkdir(parents=True, exist_ok=True)
with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as zf:
    for path in sorted(stage.rglob("*")):
        if not path.is_file():
            continue
        info = zipfile.ZipInfo.from_file(path, path.relative_to(stage).as_posix())
        info.compress_type = zipfile.ZIP_DEFLATED
        info.external_attr = (path.stat().st_mode & 0xFFFF) << 16
        with path.open("rb") as src, zf.open(info, "w") as dst:
            dst.write(src.read())
print(archive)
PY
fi

echo "Brick stage: $STAGE_ROOT"
