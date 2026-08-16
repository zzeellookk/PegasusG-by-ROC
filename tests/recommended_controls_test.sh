#!/bin/sh
set -eu

ROOT="${TMPDIR:-/tmp}/pegasusg-recommended-controls-test-$$"
APP="$ROOT/app"
RETRO="$ROOT/retro"
HOME_CFG="$ROOT/home/retroarch.cfg"
STATE="$ROOT/state"
SCRIPT="H700/launcher/apply_recommended_controls.sh"

cleanup() {
  rm -rf "$ROOT"
}
trap cleanup EXIT INT TERM

mkdir -p "$APP/assets/recommended_controls/core" \
  "$APP/assets/recommended_controls/remaps" \
  "$RETRO/config/mGBA" "$RETRO/config/gpSP" "$RETRO/config/VBA-M" \
  "$RETRO/remaps/mGBA" "$RETRO/remaps/gpSP" "$RETRO/remaps/VBA-M" \
  "$(dirname -- "$HOME_CFG")" "$STATE"
cp H700/recommended_controls/global.cfg "$APP/assets/recommended_controls/global.cfg"
cp H700/recommended_controls/core/*.cfg "$APP/assets/recommended_controls/core/"
cp H700/recommended_controls/remaps/*.rmp "$APP/assets/recommended_controls/remaps/"

printf 'input_menu_toggle_btn = "4"\nkeep_global = "yes"\n' >"$HOME_CFG"
printf 'input_rewind_btn = "3"\nkeep_vendor = "yes"\n' >"$RETRO/retroarch.cfg"
printf 'input_menu_toggle = "menu"\ninput_menu_toggle_gamepad_combo = "2"\nkeep_core = "yes"\nvideo_shader = "/custom.glslp"\n' \
  >"$RETRO/config/mGBA/mGBA.cfg"
printf 'old-remap\n' >"$RETRO/remaps/mGBA/mGBA.rmp"

run_controls() {
  PEGASUSG_APP_DIR="$APP" PEGASUSG_RETRO_ROOT="$RETRO" \
    PEGASUSG_GLOBAL_RA_CONFIG="$HOME_CFG" PEGASUSG_STATE_DIR="$STATE" \
    sh "$SCRIPT" "$1"
}

run_controls enable
grep -q 'input_menu_toggle_btn = "4"' "$HOME_CFG"
grep -q 'keep_global = "yes"' "$HOME_CFG"
grep -q 'input_rewind_btn = "9"' "$RETRO/retroarch.cfg"
grep -q 'keep_vendor = "yes"' "$RETRO/retroarch.cfg"
grep -q 'input_menu_toggle = "menu"' "$RETRO/config/mGBA/mGBA.cfg"
grep -q 'input_menu_toggle_gamepad_combo = "2"' "$RETRO/config/mGBA/mGBA.cfg"
grep -q 'keep_core = "yes"' "$RETRO/config/mGBA/mGBA.cfg"
grep -q 'video_shader = "/custom.glslp"' "$RETRO/config/mGBA/mGBA.cfg"
cmp "$RETRO/remaps/mGBA/mGBA.rmp" "$APP/assets/recommended_controls/remaps/mGBA.rmp"

# Reapplying the enabled mode refreshes managed values without duplicating keys.
printf 'unmanaged_after_enable = "kept"\n' >>"$RETRO/config/mGBA/mGBA.cfg"
run_controls enable
test "$(grep -c '^input_menu_toggle[[:space:]]*=' "$RETRO/config/mGBA/mGBA.cfg")" -eq 1
grep -q 'unmanaged_after_enable = "kept"' "$RETRO/config/mGBA/mGBA.cfg"

run_controls disable
grep -q 'input_menu_toggle_btn = "4"' "$HOME_CFG"
grep -q 'input_rewind_btn = "3"' "$RETRO/retroarch.cfg"
grep -q 'input_menu_toggle = "menu"' "$RETRO/config/mGBA/mGBA.cfg"
grep -q 'unmanaged_after_enable = "kept"' "$RETRO/config/mGBA/mGBA.cfg"
grep -q '^old-remap$' "$RETRO/remaps/mGBA/mGBA.rmp"
if grep -q 'input_hold_fast_forward_btn' "$HOME_CFG"; then
  echo 'recommended-only global key survived disable' >&2
  exit 1
fi

# Missing targets are created on enable and removed again on disable.
rm -f "$RETRO/config/gpSP/gpSP.cfg" "$RETRO/remaps/gpSP/gpSP.rmp"
run_controls enable
test -f "$RETRO/config/gpSP/gpSP.cfg"
test -f "$RETRO/remaps/gpSP/gpSP.rmp"
run_controls disable
test ! -e "$RETRO/config/gpSP/gpSP.cfg"
test ! -e "$RETRO/remaps/gpSP/gpSP.rmp"

# Upgrade from the first 1.02 build: recover the menu bindings that were saved
# before it forced button 8 globally and F1 in each core.
CONTROL_STATE="$STATE/recommended-controls"
rm -f "$CONTROL_STATE"/*.menu_migrated
printf 'input_menu_toggle_btn = "11"\ninput_menu_toggle_gamepad_combo = "2"\ninput_rewind_btn = "3"\n' \
  >"$CONTROL_STATE/global_home.saved"
printf 'input_menu_toggle = "f1"\ninput_menu_toggle_btn = "8"\ninput_menu_toggle_gamepad_combo = "7"\nkeep_global = "yes"\n' \
  >"$HOME_CFG"
printf 'input_menu_toggle = "menu"\ninput_menu_toggle_gamepad_combo = "2"\ninput_rewind = "old"\n' \
  >"$CONTROL_STATE/core_mgba.saved"
printf 'input_menu_toggle = "f1"\ninput_menu_toggle_gamepad_combo = "0"\ninput_rewind = "r"\n' \
  >"$RETRO/config/mGBA/mGBA.cfg"
run_controls enable
grep -q 'input_menu_toggle_btn = "11"' "$HOME_CFG"
grep -q 'input_menu_toggle_gamepad_combo = "2"' "$HOME_CFG"
grep -q 'input_menu_toggle = "f1"' "$HOME_CFG"
grep -q 'input_menu_toggle = "menu"' "$RETRO/config/mGBA/mGBA.cfg"
grep -q 'input_menu_toggle_gamepad_combo = "2"' "$RETRO/config/mGBA/mGBA.cfg"
grep -q 'input_rewind_btn = "9"' "$HOME_CFG"
grep -q 'input_rewind = "r"' "$RETRO/config/mGBA/mGBA.cfg"

# The actual 1.02 first-release backup can contain no core menu keys. In that
# case the injected F1/combo 0 override must simply be removed.
rm -f "$CONTROL_STATE/core_gpsp.menu_migrated"
: >"$CONTROL_STATE/core_gpsp.saved"
printf 'input_menu_toggle = "f1"\ninput_menu_toggle_gamepad_combo = "0"\ninput_rewind = "r"\n' \
  >"$RETRO/config/gpSP/gpSP.cfg"
run_controls enable
if grep -q '^input_menu_toggle[[:space:]]*=' "$RETRO/config/gpSP/gpSP.cfg"; then
  echo 'legacy core menu key survived migration' >&2
  exit 1
fi
if grep -q '^input_menu_toggle_gamepad_combo[[:space:]]*=' "$RETRO/config/gpSP/gpSP.cfg"; then
  echo 'legacy core menu combo survived migration' >&2
  exit 1
fi
