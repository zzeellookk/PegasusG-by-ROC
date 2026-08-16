#!/bin/sh
set -eu

ROOT="${TMPDIR:-/tmp}/pegasusg-auto-cheats-test-$$"
APP="$ROOT/app"
RETRO="$ROOT/retro"
GLOBAL_CONFIG="$ROOT/home/retroarch.cfg"
STATE="$ROOT/state"
SCRIPT="H700/launcher/apply_auto_cheats.sh"

cleanup() {
  rm -rf "$ROOT"
}
trap cleanup EXIT INT TERM

mkdir -p "$APP/assets/cheats" "$RETRO/cheats/mGBA" \
  "$(dirname -- "$GLOBAL_CONFIG")" "$STATE"
cp assets/cheats/gba-auto-cheats.zip "$APP/assets/cheats/"

printf 'keep_global = "yes"\napply_cheats_after_load = "false"\n' >"$GLOBAL_CONFIG"
printf 'keep_vendor = "yes"\ncheat_database_path = "/old/path"\n' >"$RETRO/retroarch.cfg"
sample="$(unzip -Z1 "$APP/assets/cheats/gba-auto-cheats.zip" | grep '^mGBA/.*\.cht$' | head -n 1)"
sample_name="${sample##*/}"
printf 'old-cheat\n' >"$RETRO/cheats/mGBA/$sample_name"

run_auto_cheats() {
  PEGASUSG_APP_DIR="$APP" PEGASUSG_RETRO_ROOT="$RETRO" \
    PEGASUSG_GLOBAL_RA_CONFIG="$GLOBAL_CONFIG" PEGASUSG_STATE_DIR="$STATE" \
    PEGASUSG_UNZIP="$(command -v unzip)" sh "$SCRIPT"
}

run_auto_cheats
test "$(find "$RETRO/cheats/gpSP" -type f -name '*.cht' | wc -l)" -eq 472
test "$(find "$RETRO/cheats/mGBA" -type f -name '*.cht' | wc -l)" -eq 472
test "$(find "$RETRO/cheats/VBA-M" -type f -name '*.cht' | wc -l)" -eq 472
test ! -e "$RETRO/cheats/VBA Next"
grep -q '^old-cheat$' "$STATE/auto-cheats/original/mGBA/$sample_name"
grep -q '^cheat_database_path = "'"$RETRO"'/cheats"$' "$GLOBAL_CONFIG"
grep -q '^apply_cheats_after_load = "true"$' "$GLOBAL_CONFIG"
grep -q '^apply_cheats_after_toggle = "true"$' "$RETRO/retroarch.cfg"
grep -q '^keep_global = "yes"$' "$GLOBAL_CONFIG"
grep -q '^keep_vendor = "yes"$' "$RETRO/retroarch.cfg"

# Re-running the same library version must preserve player-saved cheat states.
printf 'player-modified-cheat\n' >"$RETRO/cheats/mGBA/$sample_name"
run_auto_cheats
grep -q '^player-modified-cheat$' "$RETRO/cheats/mGBA/$sample_name"
