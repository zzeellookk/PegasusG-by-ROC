#!/bin/sh
set -eu

APP_DIR="${PEGASUSG_APP_DIR:-$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)}"
RETRO_ROOT="${PEGASUSG_RETRO_ROOT:-/mnt/vendor/deep/retro}"
GLOBAL_CONFIG="${PEGASUSG_GLOBAL_RA_CONFIG:-/.config/retroarch/retroarch.cfg}"
STATE_DIR="${PEGASUSG_STATE_DIR:-/mnt/data/pegasusg-by-roc}/auto-cheats"
PAYLOAD="$APP_DIR/assets/cheats/gba-auto-cheats.zip"
CHEAT_ROOT="$RETRO_ROOT/cheats"
LIBRARY_VERSION="20260814-1"
EXPECTED_PER_CORE=472
EXPECTED_TOTAL=1416
MARKER="$STATE_DIR/library.version"
BACKUP_ROOT="$STATE_DIR/original"
TEMP="$STATE_DIR/import.$$"

log_line() {
  printf '%s\n' "$*" >>"$STATE_DIR/auto-cheats.log"
}

cleanup() {
  case "$TEMP" in
    "$STATE_DIR"/import.*) [ ! -e "$TEMP" ] || rm -rf "$TEMP" ;;
  esac
}

find_unzip() {
  if [ -n "${PEGASUSG_UNZIP:-}" ]; then
    printf '%s\n' "$PEGASUSG_UNZIP"
  elif [ -x /mnt/mod/ctrl/unzip ]; then
    printf '%s\n' /mnt/mod/ctrl/unzip
  elif command -v unzip >/dev/null 2>&1; then
    command -v unzip
  else
    return 1
  fi
}

install_config() {
  target="$1"
  mkdir -p "$(dirname -- "$target")"
  temporary="$target.pegasusg_by_roc.tmp.$$"
  if [ -f "$target" ]; then
    awk '
      /^[[:space:]]*(cheat_database_path|apply_cheats_after_load|apply_cheats_after_toggle)[[:space:]]*=/ { next }
      { print }
    ' "$target" >"$temporary"
  else
    : >"$temporary"
  fi
  printf 'cheat_database_path = "%s"\n' "$CHEAT_ROOT" >>"$temporary"
  printf 'apply_cheats_after_load = "true"\n' >>"$temporary"
  printf 'apply_cheats_after_toggle = "true"\n' >>"$temporary"
  mv -f "$temporary" "$target"
  chmod 644 "$target" 2>/dev/null || true
}

install_library() {
  current="$(cat "$MARKER" 2>/dev/null || true)"
  [ "$current" = "$LIBRARY_VERSION" ] && return 0
  [ -f "$PAYLOAD" ] || return 1
  unzip_command="$(find_unzip)" || return 1

  cleanup
  mkdir -p "$TEMP" "$CHEAT_ROOT" "$BACKUP_ROOT"
  "$unzip_command" -oq "$PAYLOAD" -d "$TEMP"

  actual_total="$(find "$TEMP" -type f -name '*.cht' | wc -l | tr -d '[:space:]')"
  [ "$actual_total" = "$EXPECTED_TOTAL" ] || return 1
  if find "$TEMP" -type f ! -name '*.cht' | grep -q .; then return 1; fi
  if find "$TEMP" -type l | grep -q .; then return 1; fi

  for core in gpSP mGBA VBA-M; do
    source_dir="$TEMP/$core"
    target_dir="$CHEAT_ROOT/$core"
    backup_dir="$BACKUP_ROOT/$core"
    actual_core="$(find "$source_dir" -type f -name '*.cht' | wc -l | tr -d '[:space:]')"
    [ "$actual_core" = "$EXPECTED_PER_CORE" ] || return 1
    mkdir -p "$target_dir" "$backup_dir"
    find "$source_dir" -type f -name '*.cht' | while IFS= read -r source; do
      name="${source##*/}"
      target="$target_dir/$name"
      backup="$backup_dir/$name"
      if [ -f "$target" ] && [ ! -e "$backup" ]; then
        cp -p "$target" "$backup"
      fi
      temporary="$target.pegasusg_by_roc.tmp.$$"
      cp "$source" "$temporary"
      chmod 644 "$temporary" 2>/dev/null || true
      mv -f "$temporary" "$target"
    done
  done

  printf '%s\n' "$LIBRARY_VERSION" >"$MARKER"
  cleanup
  log_line "installed library=$LIBRARY_VERSION cheats=$EXPECTED_TOTAL"
}

mkdir -p "$STATE_DIR"
trap cleanup 0 1 2 15
install_library
install_config "$GLOBAL_CONFIG"
install_config "$RETRO_ROOT/retroarch.cfg"
trap - 0 1 2 15
