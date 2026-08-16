#!/bin/sh
set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
PAYLOAD="$SCRIPT_DIR/gba-cheats.zip"
EXPECTED_CHEATS=2801

RETRO_ROOT="${PEGASUSG_RETRO_ROOT:-/mnt/vendor/deep/retro}"
STATE_ROOT="${PEGASUSG_STATE_ROOT:-/mnt/data/pegasusg-by-roc}"
CHEAT_ROOT="${PEGASUSG_CHEAT_ROOT:-$RETRO_ROOT/cheats}"
TARGET="$CHEAT_ROOT/Nintendo - Game Boy Advance"
BACKUP="$CHEAT_ROOT/.pegasusg_by_roc_gba_cheats_original"
TEMP="$CHEAT_ROOT/.pegasusg_by_roc_gba_import.$$"
LOG_FILE="$STATE_ROOT/cheat-importer.log"

mkdir -p "$STATE_ROOT"

log_line() {
  printf '%s\n' "$*"
  printf '%s\n' "$*" >> "$LOG_FILE"
}

fail() {
  log_line "[cheats] ERROR: $*"
  exit 1
}

cleanup() {
  case "$TEMP" in
    "$CHEAT_ROOT"/.pegasusg_by_roc_gba_import.*)
      [ ! -e "$TEMP" ] || rm -rf "$TEMP"
      ;;
  esac
}

trap cleanup 0 1 2 15

[ -d "$RETRO_ROOT" ] || fail "RetroArch root not found: $RETRO_ROOT"
[ -f "$PAYLOAD" ] || fail "Missing gba-cheats.zip"
mkdir -p "$CHEAT_ROOT"

case "$TARGET" in
  "$CHEAT_ROOT/Nintendo - Game Boy Advance") ;;
  *) fail "Unsafe target path: $TARGET" ;;
esac

if [ -n "${PEGASUSG_UNZIP:-}" ]; then
  UNZIP="$PEGASUSG_UNZIP"
elif [ -x /mnt/mod/ctrl/unzip ]; then
  UNZIP=/mnt/mod/ctrl/unzip
elif command -v unzip >/dev/null 2>&1; then
  UNZIP="$(command -v unzip)"
else
  fail "No unzip tool found"
fi

[ ! -e "$TEMP" ] || fail "Temporary path already exists: $TEMP"
mkdir "$TEMP"
log_line "[cheats] Extracting PegasusG by ROC GBA cheat library"
"$UNZIP" -oq "$PAYLOAD" -d "$TEMP" || fail "Could not extract cheat library"

ACTUAL_CHEATS="$(find "$TEMP" -type f -name '*.cht' | wc -l | tr -d '[:space:]')"
[ "$ACTUAL_CHEATS" = "$EXPECTED_CHEATS" ] || fail "Expected $EXPECTED_CHEATS cheat files, found $ACTUAL_CHEATS"

if find "$TEMP" -type l | grep -q .; then
  fail "Cheat payload contains symbolic links"
fi

if [ ! -e "$BACKUP" ] && { [ -e "$TARGET" ] || [ -L "$TARGET" ]; }; then
  mv "$TARGET" "$BACKUP"
elif [ -e "$TARGET" ] || [ -L "$TARGET" ]; then
  rm -rf "$TARGET"
fi

mv "$TEMP" "$TARGET"
trap - 0 1 2 15
chmod -R a+rX "$TARGET" 2>/dev/null || true
printf '%s\n' "$EXPECTED_CHEATS" > "$STATE_ROOT/imported-gba-cheats.txt"
sync
log_line "[cheats] Imported $EXPECTED_CHEATS GBA cheat files for RetroArch."
