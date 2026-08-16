#!/bin/sh
set -eu

ROOT="${TMPDIR:-/tmp}/pegasusg-game-overrides-test-$$"
APP="$ROOT/app"
RETRO="$ROOT/retro"
CONFIG="$ROOT/config"
STATE="$ROOT/state"
SCRIPT="H700/launcher/apply_game_overrides.sh"
MMC_ROM="$ROOT/mnt/mmc/Roms/GBA/火焰纹章 埃力格之枪.gba"
SD_ROM="$ROOT/mnt/sdcard/Roms/GBA hack/口袋妖怪 龙珠Z 超激战.zip"

cleanup() {
  rm -rf "$ROOT"
}
trap cleanup EXIT INT TERM

mkdir -p "$APP/assets/core_options/mGBA" "$RETRO/config/mGBA" "$CONFIG" "$STATE" \
  "$(dirname -- "$MMC_ROM")" "$(dirname -- "$SD_ROM")"
cp H700/core_options/mGBA/*.opt "$APP/assets/core_options/mGBA/"
: >"$MMC_ROM"
: >"$SD_ROM"
printf 'keep_global = "yes"\nsavefile_directory = "/old/global/saves"\nsavestate_directory = "/old/global/states"\n' >"$CONFIG/retroarch.cfg"
printf 'keep_gba = "yes"\nsavefile_directory = "/old/gba/saves"\nsavestate_directory = "/old/gba/states"\n' >"$CONFIG/retroarch_GBA.cfg"

run_override() {
  mapped_rom="${2:-}"
  if [ -n "$mapped_rom" ]; then
    case "$mapped_rom" in
      "$ROOT"/mnt/mmc/*) mapped_rom="/mnt/mmc/${mapped_rom#"$ROOT/mnt/mmc/"}" ;;
      "$ROOT"/mnt/sdcard/*) mapped_rom="/mnt/sdcard/${mapped_rom#"$ROOT/mnt/sdcard/"}" ;;
    esac
  fi
  PEGASUSG_APP_DIR="$APP" PEGASUSG_RETRO_ROOT="$RETRO" \
    PEGASUSG_GLOBAL_RA_CONFIG="$CONFIG/retroarch.cfg" \
    PEGASUSG_GBA_RA_CONFIG="$CONFIG/retroarch_GBA.cfg" \
    PEGASUSG_STATE_DIR="$STATE" PEGASUSG_MGBA_OPTION_DIR="$RETRO/config/mGBA" \
    PEGASUSG_TEST_PATH_PREFIX="$ROOT" sh "$SCRIPT" "$1" ${mapped_rom:+"$mapped_rom"}
}

# Use a mirrored /mnt tree when the test cannot create real mount paths.
prepare_with_test_path() {
  action="$1"
  rom="$2"
  mapped="${rom#"$ROOT"}"
  PEGASUSG_APP_DIR="$APP" PEGASUSG_RETRO_ROOT="$RETRO" \
    PEGASUSG_GLOBAL_RA_CONFIG="$CONFIG/retroarch.cfg" \
    PEGASUSG_GBA_RA_CONFIG="$CONFIG/retroarch_GBA.cfg" \
    PEGASUSG_STATE_DIR="$STATE" PEGASUSG_MGBA_OPTION_DIR="$RETRO/config/mGBA" \
    PEGASUSG_PATH_ROOT="$ROOT" sh "$SCRIPT" "$action" "$mapped"
}

run_override install-core-options
test -f "$RETRO/config/mGBA/火焰纹章 埃力格之枪.opt"
test -f "$RETRO/config/mGBA/口袋妖怪 龙珠Z 超激战.opt"
grep -q '^mgba_skip_bios = "ON"$' "$RETRO/config/mGBA/火焰纹章 埃力格之枪.opt"

prepare_with_test_path prepare-saves "$MMC_ROM"
grep -q '^savefile_directory = "/mnt/mmc/Roms/GBA"$' "$CONFIG/retroarch.cfg"
grep -q '^savestate_directory = "/mnt/mmc/Roms/GBA"$' "$CONFIG/retroarch_GBA.cfg"
printf 'player_setting = "kept"\n' >>"$CONFIG/retroarch_GBA.cfg"
run_override restore-saves
grep -q '^savefile_directory = "/old/global/saves"$' "$CONFIG/retroarch.cfg"
grep -q '^savestate_directory = "/old/gba/states"$' "$CONFIG/retroarch_GBA.cfg"
grep -q '^player_setting = "kept"$' "$CONFIG/retroarch_GBA.cfg"

prepare_with_test_path prepare-saves "$SD_ROM"
grep -q '^savefile_directory = "/mnt/sdcard/Roms/GBA hack"$' "$CONFIG/retroarch.cfg"
grep -q '^savestate_directory = "/mnt/sdcard/Roms/GBA hack"$' "$CONFIG/retroarch_GBA.cfg"

# A second prepare restores a stale redirect before applying the next ROM path.
prepare_with_test_path prepare-saves "$MMC_ROM"
grep -q '^savefile_directory = "/mnt/mmc/Roms/GBA"$' "$CONFIG/retroarch_GBA.cfg"
run_override restore-saves
grep -q '^savefile_directory = "/old/gba/saves"$' "$CONFIG/retroarch_GBA.cfg"
