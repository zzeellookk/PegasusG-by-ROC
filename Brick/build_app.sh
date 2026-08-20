#!/bin/bash
set -euo pipefail

SELF_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "$SELF_DIR/.." && pwd)"

VERSION="${PEGASUSG_VERSION:-1.1.1-hotfix3-brick}"
OUTPUT="${PEGASUSG_OUTPUT:-Stage}"
SYSROOT="${PEGASUSG_SYSROOT:-$SELF_DIR/sysroot}"
BUILD_DIR="$SELF_DIR/build"
DIST_ROOT="$SELF_DIR/dist"
FULL_STAGE_ROOT="$DIST_ROOT/full-stage"
UPDATE_STAGE_ROOT="$DIST_ROOT/update-stage"
FULL_APP_DIR="$FULL_STAGE_ROOT/Apps/PegasusG"
UPDATE_APP_DIR="$UPDATE_STAGE_ROOT/Apps/PegasusG"
FULL_ARCHIVE="$DIST_ROOT/PegasusG-Brick-${VERSION}-Full.zip"
UPDATE_ARCHIVE="$DIST_ROOT/PegasusG-Brick-${VERSION}-Update.zip"
CORE_SOURCE_DIR="$SELF_DIR/cores"

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

mkdir -p "$BUILD_DIR"
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

rm -rf "$FULL_STAGE_ROOT" "$UPDATE_STAGE_ROOT"
mkdir -p "$FULL_APP_DIR" "$UPDATE_APP_DIR"

stage_runtime() {
  app_dir="$1"
  mkdir -p "$app_dir/cores"
  cp "$BUILD_DIR/pegasusg_by_roc" "$app_dir/pegasusg_by_roc"
  cp "$SELF_DIR/launcher/launch.sh" "$app_dir/launch.sh"
  cp "$SELF_DIR/launcher/autostart_ctl.sh" "$app_dir/autostart_ctl.sh"
  cp "$SELF_DIR/launcher/autostart_launch.sh" "$app_dir/autostart_launch.sh"
  for core in mgba_libretro.so gpsp_libretro.so vbam_libretro.so vba_next_libretro.so; do
    [ -f "$CORE_SOURCE_DIR/$core" ] || {
      echo "missing Brick AArch64 core: $CORE_SOURCE_DIR/$core" >&2
      exit 4
    }
    cp "$CORE_SOURCE_DIR/$core" "$app_dir/cores/$core"
  done
  [ ! -f "$CORE_SOURCE_DIR/README.md" ] || cp "$CORE_SOURCE_DIR/README.md" "$app_dir/cores/README.md"
  printf '%s\n' "$VERSION" > "$app_dir/version.txt"
  chmod 755 "$app_dir/pegasusg_by_roc" "$app_dir/"*.sh
  chmod 644 "$app_dir/cores/"*.so
}

stage_runtime "$FULL_APP_DIR"
stage_runtime "$UPDATE_APP_DIR"

mkdir -p "$FULL_APP_DIR/assets/music" "$FULL_APP_DIR/assets/ui" \
  "$FULL_APP_DIR/config"
cp "$SELF_DIR/launcher/config.json" "$FULL_APP_DIR/config.json"
cp "$SELF_DIR/launcher/mods.txt" "$FULL_APP_DIR/config/mods.txt"
cp "$REPO_ROOT/assets/ui/pegasusg_app_entry.png" "$FULL_APP_DIR/icon.png"
cp "$REPO_ROOT/assets/ui/pegasus_g.png" "$FULL_APP_DIR/assets/ui/pegasus_g.png"
cp "$REPO_ROOT/assets/music/builtin/"*.mp3 "$FULL_APP_DIR/assets/music/"
cp "$REPO_ROOT/LICENSE.md" "$FULL_APP_DIR/LICENSE.md"
cp "$REPO_ROOT/NOTICE.md" "$FULL_APP_DIR/NOTICE.md"
cp "$REPO_ROOT/NOTICE.zh-CN.md" "$FULL_APP_DIR/NOTICE.zh-CN.md"
cp "$REPO_ROOT/THIRD_PARTY_NOTICES.md" "$FULL_APP_DIR/THIRD_PARTY_NOTICES.md"
cp "$SELF_DIR/package/README.zh-CN.md" "$FULL_APP_DIR/README.zh-CN.md"

python3 - "$FULL_STAGE_ROOT" "$UPDATE_STAGE_ROOT" "$FULL_ARCHIVE" \
  "$UPDATE_ARCHIVE" "$OUTPUT" <<'PY'
import pathlib
import sys
import zipfile

full_stage = pathlib.Path(sys.argv[1])
update_stage = pathlib.Path(sys.argv[2])
full_archive = pathlib.Path(sys.argv[3])
update_archive = pathlib.Path(sys.argv[4])
output = sys.argv[5]
full_app = full_stage / "Apps" / "PegasusG"
update_app = update_stage / "Apps" / "PegasusG"

common = [
    "pegasusg_by_roc",
    "launch.sh",
    "autostart_ctl.sh",
    "autostart_launch.sh",
    "version.txt",
    "cores/mgba_libretro.so",
    "cores/gpsp_libretro.so",
    "cores/vbam_libretro.so",
    "cores/vba_next_libretro.so",
]
full_required = common + [
    "config.json",
    "config/mods.txt",
    "icon.png",
    "assets/ui/pegasus_g.png",
    "LICENSE.md",
    "NOTICE.md",
    "NOTICE.zh-CN.md",
    "THIRD_PARTY_NOTICES.md",
    "README.zh-CN.md",
]
for app, required, label in (
    (full_app, full_required, "full"),
    (update_app, common, "update"),
):
    missing = [name for name in required if not (app / name).is_file()]
    if missing:
        raise SystemExit(f"[brick] {label} package missing: {', '.join(missing)}")

music = sorted((full_app / "assets" / "music").glob("*.mp3"))
if len(music) != 11:
    raise SystemExit(f"[brick] full package expected 11 music tracks, found {len(music)}")
for forbidden in ("ffmpeg", "tools"):
    if (full_app / forbidden).exists():
        raise SystemExit(f"[brick] stable full package contains forbidden experimental path: {forbidden}")
if (full_stage / "System" / "starts" / "zz_pegasusg_autostart.sh").exists():
    raise SystemExit("[brick] full package must not enable autostart during installation")

def write_zip(stage: pathlib.Path, archive: pathlib.Path) -> None:
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

if output == "Zip":
    write_zip(full_stage, full_archive)
    write_zip(update_stage, update_archive)
    print(full_archive)
    print(update_archive)
PY

echo "Brick full stage: $FULL_STAGE_ROOT"
echo "Brick update stage: $UPDATE_STAGE_ROOT"
