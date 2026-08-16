#!/bin/sh
set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
VARIANT_FILE="$SCRIPT_DIR/variant.txt"
PRESET_SOURCE="$SCRIPT_DIR/preset.glslp"
SHADER_SOURCE="$SCRIPT_DIR/shaders"

RETRO_ROOT="${PEGASUSG_RETRO_ROOT:-/mnt/vendor/deep/retro}"
STATE_ROOT="${PEGASUSG_STATE_ROOT:-/mnt/data/pegasusg-by-roc}"
CONFIG_ROOT="$RETRO_ROOT/config"
TARGET_ROOT="$RETRO_ROOT/shaders/pegasusg_by_roc_gba"
TARGET_PRESET="$TARGET_ROOT/current.glslp"
LOG_FILE="$STATE_ROOT/filter-installer.log"

mkdir -p "$STATE_ROOT"

log_line() {
  printf '%s\n' "$*"
  printf '%s\n' "$*" >> "$LOG_FILE"
}

fail() {
  log_line "[filter] ERROR: $*"
  exit 1
}

[ -d "$RETRO_ROOT" ] || fail "RetroArch root not found: $RETRO_ROOT"
[ -f "$VARIANT_FILE" ] || fail "Missing variant.txt"
[ -f "$PRESET_SOURCE" ] || fail "Missing preset.glslp"
[ -d "$SHADER_SOURCE" ] || fail "Missing shaders directory"

VARIANT="$(tr -d '\r\n' < "$VARIANT_FILE")"
case "$VARIANT" in
  calibrated|original) ;;
  *) fail "Unsupported filter variant: $VARIANT" ;;
esac

backup_once() {
  path="$1"
  backup="${path}.pegasusg_by_roc.original"
  if [ -f "$path" ] && [ ! -e "$backup" ]; then
    cp -p "$path" "$backup"
  fi
}

update_core_config() {
  cfg="$1"
  tmp="${cfg}.pegasusg_by_roc.tmp.$$"

  backup_once "$cfg"
  if [ -f "$cfg" ]; then
    sed \
      -e '/^[[:space:]]*video_shader_enable[[:space:]]*=/d' \
      -e '/^[[:space:]]*video_shader[[:space:]]*=/d' \
      "$cfg" > "$tmp"
  else
    : > "$tmp"
  fi

  printf 'video_shader_enable = "true"\n' >> "$tmp"
  printf 'video_shader = "%s"\n' "$TARGET_PRESET" >> "$tmp"
  mv -f "$tmp" "$cfg"
  chmod 644 "$cfg" 2>/dev/null || true
}

write_auto_preset() {
  preset="$1"
  tmp="${preset}.pegasusg_by_roc.tmp.$$"

  backup_once "$preset"
  printf '#reference "%s"\n' "$TARGET_PRESET" > "$tmp"
  mv -f "$tmp" "$preset"
  chmod 644 "$preset" 2>/dev/null || true
}

install_for_core() {
  core_dir="$1"
  core_name="$2"
  dir="$CONFIG_ROOT/$core_dir"

  mkdir -p "$dir"
  update_core_config "$dir/$core_name.cfg"
  write_auto_preset "$dir/$core_name.glslp"
  write_auto_preset "$dir/GBA.glslp"
}

log_line "[filter] Installing PegasusG by ROC GBA filter: $VARIANT"
mkdir -p "$TARGET_ROOT/shaders"
cp -f "$PRESET_SOURCE" "$TARGET_PRESET"
cp -f "$SHADER_SOURCE"/*.glsl "$TARGET_ROOT/shaders/"
chmod 644 "$TARGET_PRESET" "$TARGET_ROOT/shaders"/*.glsl 2>/dev/null || true

install_for_core "mGBA" "mGBA"
install_for_core "gpSP" "gpSP"

printf '%s\n' "$VARIANT" > "$STATE_ROOT/installed-gba-filter.txt"
sync
log_line "[filter] Installed for mGBA and gpSP. It will apply on the next GBA launch."
