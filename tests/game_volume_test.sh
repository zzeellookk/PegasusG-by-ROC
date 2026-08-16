#!/bin/sh
set -eu

ROOT="${TMPDIR:-/tmp}/pegasusg-game-volume-test-$$"
STATE="$ROOT/state"
RA_VOLUME="$ROOT/retroarch_volume.cfg"
SCRIPT="H700/launcher/game_volume.sh"

cleanup() {
  rm -rf "$ROOT"
}
trap cleanup EXIT INT TERM
mkdir -p "$STATE"

run_volume() {
  PEGASUSG_STATE_DIR="$STATE" PEGASUSG_RA_VOLUME_CFG="$RA_VOLUME" sh "$SCRIPT" "$1"
}

printf '2\n' >"$STATE/volume.level"
printf 'audio_volume = "0.0"\n' >"$RA_VOLUME"
run_volume prepare
grep -q '^audio_volume = "-15.8"$' "$RA_VOLUME"
grep -q '^-15.8$' "$STATE/game-volume.db"

# A volume selected inside RetroArch persists independently from frontend volume.
printf 'audio_volume = "-3.0"\n' >"$RA_VOLUME"
run_volume capture
printf 'audio_volume = "0.0"\n' >"$RA_VOLUME"
run_volume prepare
grep -q '^audio_volume = "-3.0"$' "$RA_VOLUME"

# Upgrade an old migrated installation without applying attenuation twice.
rm -f "$STATE/game-volume.db" "$STATE/game-volume.schema"
: >"$STATE/game-volume-independent"
printf 'audio_volume = "-8.2"\n' >"$RA_VOLUME"
run_volume prepare
grep -q '^audio_volume = "-8.2"$' "$RA_VOLUME"

# An old 0 dB state is rebased once to the current low frontend volume.
rm -f "$STATE/game-volume.db" "$STATE/game-volume.schema"
printf 'audio_volume = "0.0"\n' >"$RA_VOLUME"
run_volume prepare
grep -q '^audio_volume = "-15.8"$' "$RA_VOLUME"
