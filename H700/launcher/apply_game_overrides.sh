#!/bin/sh
set -eu

APP_DIR="${PEGASUSG_APP_DIR:-$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)}"
RETRO_ROOT="${PEGASUSG_RETRO_ROOT:-/mnt/vendor/deep/retro}"
GLOBAL_CONFIG="${PEGASUSG_GLOBAL_RA_CONFIG:-/.config/retroarch/retroarch.cfg}"
GBA_CONFIG="${PEGASUSG_GBA_RA_CONFIG:-/.config/retroarch/retroarch_GBA.cfg}"
STATE_ROOT="${PEGASUSG_STATE_DIR:-/mnt/data/pegasusg-by-roc}"
SESSION_DIR="$STATE_ROOT/ra-save-redirect"
OPTION_SOURCE="$APP_DIR/assets/core_options/mGBA"
OPTION_TARGET="${PEGASUSG_MGBA_OPTION_DIR:-$RETRO_ROOT/config/mGBA}"
OPTION_BACKUP="$STATE_ROOT/core-option-backups/mGBA"

log_line() {
  mkdir -p "$STATE_ROOT/logs"
  printf '%s\n' "$*" >>"$STATE_ROOT/logs/game-overrides.log"
}

install_core_options() {
  [ -d "$OPTION_SOURCE" ] || return 1
  mkdir -p "$OPTION_TARGET" "$OPTION_BACKUP"
  found=0
  for source in "$OPTION_SOURCE"/*.opt; do
    [ -f "$source" ] || continue
    found=1
    name="${source##*/}"
    target="$OPTION_TARGET/$name"
    backup="$OPTION_BACKUP/$name.original"
    if [ -f "$target" ] && [ ! -e "$backup" ]; then
      cp -p "$target" "$backup"
    fi
    if [ -f "$target" ] && cmp -s "$source" "$target"; then
      continue
    fi
    temporary="$target.pegasusg_by_roc.tmp.$$"
    cp "$source" "$temporary"
    chmod 644 "$temporary" 2>/dev/null || true
    mv -f "$temporary" "$target"
    log_line "installed mGBA game option: $name"
  done
  [ "$found" -eq 1 ]
}

ensure_config() {
  target="$1"
  seed="$2"
  [ -f "$target" ] && return 0
  mkdir -p "$(dirname -- "$target")"
  if [ -f "$seed" ]; then
    cp -p "$seed" "$target"
  else
    : >"$target"
  fi
}

snapshot_config() {
  label="$1"
  target="$2"
  mkdir -p "$SESSION_DIR/$label"
  for key in savefile_directory savestate_directory; do
    grep -m 1 "^[[:space:]]*$key[[:space:]]*=" "$target" \
      >"$SESSION_DIR/$label/$key.line" 2>/dev/null || :
  done
}

write_save_paths() {
  target="$1"
  save_dir="$2"
  temporary="$target.pegasusg_by_roc.tmp.$$"
  awk '
    /^[[:space:]]*(savefile_directory|savestate_directory)[[:space:]]*=/ { next }
    { print }
  ' "$target" >"$temporary"
  printf 'savefile_directory = "%s"\n' "$save_dir" >>"$temporary"
  printf 'savestate_directory = "%s"\n' "$save_dir" >>"$temporary"
  chmod --reference="$target" "$temporary" 2>/dev/null || chmod 644 "$temporary" 2>/dev/null || true
  mv -f "$temporary" "$target"
}

restore_one_config() {
  label="$1"
  target="$2"
  [ -d "$SESSION_DIR/$label" ] || return 0
  [ -f "$target" ] || return 0
  temporary="$target.pegasusg_by_roc.tmp.$$"
  awk '
    /^[[:space:]]*(savefile_directory|savestate_directory)[[:space:]]*=/ { next }
    { print }
  ' "$target" >"$temporary"
  for key in savefile_directory savestate_directory; do
    saved="$SESSION_DIR/$label/$key.line"
    [ ! -s "$saved" ] || cat "$saved" >>"$temporary"
  done
  chmod --reference="$target" "$temporary" 2>/dev/null || chmod 644 "$temporary" 2>/dev/null || true
  mv -f "$temporary" "$target"
}

restore_saves() {
  [ -d "$SESSION_DIR" ] || return 0
  restore_one_config global "$GLOBAL_CONFIG"
  restore_one_config gba "$GBA_CONFIG"
  rm -rf "$SESSION_DIR"
  log_line "restored RetroArch save directories"
}

prepare_saves() {
  rom="$1"
  case "$rom" in
    /mnt/mmc/*|/mnt/sdcard/*) ;;
    *) return 2 ;;
  esac
  case "$rom" in *'"'*) return 2 ;; esac
  save_dir="${rom%/*}"
  physical_save_dir="${PEGASUSG_PATH_ROOT:-}$save_dir"
  [ -d "$physical_save_dir" ] || return 3
  [ -w "$physical_save_dir" ] || return 4

  restore_saves
  ensure_config "$GLOBAL_CONFIG" "$RETRO_ROOT/retroarch.cfg"
  ensure_config "$GBA_CONFIG" "$GLOBAL_CONFIG"
  mkdir -p "$SESSION_DIR"
  snapshot_config global "$GLOBAL_CONFIG"
  snapshot_config gba "$GBA_CONFIG"
  write_save_paths "$GLOBAL_CONFIG" "$save_dir"
  write_save_paths "$GBA_CONFIG" "$save_dir"
  printf '%s\n' "$rom" >"$SESSION_DIR/rom.path"
  log_line "redirected RetroArch saves to ROM directory: $save_dir"
}

case "${1:-}" in
  install-core-options) install_core_options ;;
  prepare-saves)
    [ "$#" -eq 2 ] || exit 2
    prepare_saves "$2"
    ;;
  restore-saves) restore_saves ;;
  *)
    echo "usage: $0 install-core-options | prepare-saves ROM | restore-saves" >&2
    exit 2
    ;;
esac
