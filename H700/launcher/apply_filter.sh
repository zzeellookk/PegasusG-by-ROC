#!/bin/sh
set -eu

MODE="${1:-calibrated}"
CORE_CHOICE="${2:-mgba}"
APP_DIR="${PEGASUSG_APP_DIR:-$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)}"
RETRO_ROOT="${PEGASUSG_RETRO_ROOT:-/mnt/vendor/deep/retro}"
STATE_DIR="${PEGASUSG_STATE_DIR:-/mnt/data/pegasusg-by-roc}"
ASSET_ROOT="$APP_DIR/assets/filters"
TARGET_ROOT="$RETRO_ROOT/shaders/pegasusg_by_roc_gba"
TARGET_PRESET="$TARGET_ROOT/current.glslp"
CONFIG_ROOT="$RETRO_ROOT/config"
LOG_FILE="$STATE_DIR/logs/PegasusG-by-ROC.log"

mkdir -p "$STATE_DIR/logs"

log_line() {
  printf '%s\n' "$*" >>"$LOG_FILE"
}

remember_custom_file() {
  path="$1"
  custom="${path}.pegasusg_by_roc.custom"
  missing="${custom}.missing"
  legacy="${path}.pegasusg_by_roc.original"
  migrated="${legacy}.migrated"
  [ -e "$custom" ] || [ -e "$missing" ] || {
    if [ -f "$legacy" ] && [ ! -e "$migrated" ]; then
      cp -p "$legacy" "$custom"
      : >"$migrated"
    elif [ -f "$path" ]; then
      cp -p "$path" "$custom"
    else
      : >"$missing"
    fi
  }
}

restore_custom_file() {
  path="$1"
  custom="${path}.pegasusg_by_roc.custom"
  missing="${custom}.missing"
  legacy="${path}.pegasusg_by_roc.original"
  migrated="${legacy}.migrated"
  if [ -f "$custom" ]; then
    mkdir -p "$(dirname -- "$path")"
    cp -p "$custom" "$path"
    rm -f "$custom" "$missing"
  elif [ -e "$missing" ]; then
    rm -f "$path" "$missing"
  elif [ -f "$legacy" ] && [ ! -e "$migrated" ]; then
    cp -p "$legacy" "$path"
    : >"$migrated"
  fi
}

remember_custom_shader_config() {
  path="$1"
  shader_custom="${path}.pegasusg_by_roc.shader_custom"
  ready="${shader_custom}.ready"
  custom="${path}.pegasusg_by_roc.custom"
  custom_migrated="${custom}.shader_migrated"
  legacy="${path}.pegasusg_by_roc.original"
  legacy_migrated="${legacy}.migrated"
  [ -e "$ready" ] && return 0
  if [ -f "$custom" ] && [ ! -e "$custom_migrated" ]; then
    source="$custom"
    : >"$custom_migrated"
  elif [ -f "$legacy" ] && [ ! -e "$legacy_migrated" ]; then
    source="$legacy"
    : >"$legacy_migrated"
  elif [ -f "$path" ]; then
    source="$path"
  else
    source=""
    : >"${shader_custom}.target_missing"
  fi
  if [ -n "$source" ]; then
    sed -n \
      -e '/^[[:space:]]*video_shader_enable[[:space:]]*=/p' \
      -e '/^[[:space:]]*video_shader[[:space:]]*=/p' \
      "$source" >"$shader_custom"
  else
    : >"$shader_custom"
  fi
  : >"$ready"
}

write_core_config() {
  path="$1"
  tmp="${path}.pegasusg_by_roc.tmp.$$"
  remember_custom_shader_config "$path"
  if [ -f "$path" ]; then
    sed \
      -e '/^[[:space:]]*video_shader_enable[[:space:]]*=/d' \
      -e '/^[[:space:]]*video_shader[[:space:]]*=/d' \
      "$path" >"$tmp"
  else
    : >"$tmp"
  fi
  printf 'video_shader_enable = "true"\n' >>"$tmp"
  printf 'video_shader = "%s"\n' "$TARGET_PRESET" >>"$tmp"
  mv -f "$tmp" "$path"
  chmod 644 "$path" 2>/dev/null || true
}

restore_custom_shader_config() {
  path="$1"
  shader_custom="${path}.pegasusg_by_roc.shader_custom"
  ready="${shader_custom}.ready"
  [ -e "$ready" ] || return 0
  missing="${shader_custom}.target_missing"
  tmp="${path}.pegasusg_by_roc.tmp.$$"
  if [ -f "$path" ]; then
    sed \
      -e '/^[[:space:]]*video_shader_enable[[:space:]]*=/d' \
      -e '/^[[:space:]]*video_shader[[:space:]]*=/d' \
      "$path" >"$tmp"
  else
    : >"$tmp"
  fi
  cat "$shader_custom" >>"$tmp"
  if [ -e "$missing" ] && [ ! -s "$tmp" ]; then
    rm -f "$tmp" "$path"
  else
    mv -f "$tmp" "$path"
    chmod 644 "$path" 2>/dev/null || true
  fi
  rm -f "$shader_custom" "$ready" "$missing"
}

write_auto_preset() {
  path="$1"
  tmp="${path}.pegasusg_by_roc.tmp.$$"
  remember_custom_file "$path"
  printf '#reference "%s"\n' "$TARGET_PRESET" >"$tmp"
  mv -f "$tmp" "$path"
  chmod 644 "$path" 2>/dev/null || true
}

case "$CORE_CHOICE" in
  mgba) CORE_DIR='mGBA'; CORE_NAME='mGBA' ;;
  gpsp|gpsp_rumble) CORE_DIR='gpSP'; CORE_NAME='gpSP' ;;
  vbam) CORE_DIR='VBA-M'; CORE_NAME='VBA-M' ;;
  vba_next) CORE_DIR='VBA Next'; CORE_NAME='VBA Next' ;;
  *) CORE_DIR='mGBA'; CORE_NAME='mGBA' ;;
esac
CORE_CONFIG_DIR="$CONFIG_ROOT/$CORE_DIR"
CORE_CFG="$CORE_CONFIG_DIR/$CORE_NAME.cfg"
CORE_PRESET="$CORE_CONFIG_DIR/$CORE_NAME.glslp"
CONTENT_PRESET="$CORE_CONFIG_DIR/GBA.glslp"

case "$MODE" in
  custom)
    restore_custom_shader_config "$CORE_CFG"
    restore_custom_file "$CORE_PRESET"
    restore_custom_file "$CONTENT_PRESET"
    printf '%s\n' custom >"$STATE_DIR/active-gba-filter.txt"
    log_line "[filter] custom mode core=$CORE_CHOICE; managed shader override disabled"
    exit 0
    ;;
  calibrated|original) ;;
  *)
    log_line "[filter] invalid mode=$MODE; falling back to calibrated"
    MODE=calibrated
    ;;
esac

PRESET_SOURCE="$ASSET_ROOT/$MODE.glslp"
if [ ! -f "$PRESET_SOURCE" ] || [ ! -d "$ASSET_ROOT/shaders" ]; then
  log_line "[filter] assets missing for mode=$MODE"
  exit 1
fi

mkdir -p "$TARGET_ROOT/shaders" "$CORE_CONFIG_DIR"
cp -f "$PRESET_SOURCE" "$TARGET_PRESET"
cp -f "$ASSET_ROOT/shaders"/*.glsl "$TARGET_ROOT/shaders/"
chmod 644 "$TARGET_PRESET" "$TARGET_ROOT/shaders"/*.glsl 2>/dev/null || true
write_core_config "$CORE_CFG"
write_auto_preset "$CORE_PRESET"
write_auto_preset "$CONTENT_PRESET"
printf '%s\n' "$MODE" >"$STATE_DIR/active-gba-filter.txt"
log_line "[filter] applied mode=$MODE core=$CORE_CHOICE"
