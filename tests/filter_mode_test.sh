#!/bin/sh
set -eu

ROOT="${TMPDIR:-/tmp}/pegasusg-filter-mode-test-$$"
APP="$ROOT/app"
RETRO="$ROOT/retro"
STATE="$ROOT/state"
SCRIPT="H700/launcher/apply_filter.sh"

cleanup() {
  rm -rf "$ROOT"
}
trap cleanup EXIT INT TERM

mkdir -p "$APP/assets/filters/shaders" "$RETRO/config/mGBA" "$STATE"
cp H700/filter_injectors/calibrated/preset.glslp "$APP/assets/filters/calibrated.glslp"
cp H700/filter_injectors/original/preset.glslp "$APP/assets/filters/original.glslp"
cp H700/filter_injectors/common/shaders/*.glsl "$APP/assets/filters/shaders/"

CFG="$RETRO/config/mGBA/mGBA.cfg"
CORE_PRESET="$RETRO/config/mGBA/mGBA.glslp"
CONTENT_PRESET="$RETRO/config/mGBA/GBA.glslp"
printf 'video_shader_enable = "false"\nvideo_shader = "/custom/first.glslp"\ninput_overlay = "/custom/overlay.cfg"\nkeep = "yes"\n' >"$CFG"
printf '#reference "/custom/core.glslp"\n' >"$CORE_PRESET"
printf '#reference "/custom/content.glslp"\n' >"$CONTENT_PRESET"

run_filter() {
  PEGASUSG_APP_DIR="$APP" PEGASUSG_RETRO_ROOT="$RETRO" \
    PEGASUSG_STATE_DIR="$STATE" sh "$SCRIPT" "$1" "${2:-mgba}"
}

run_filter calibrated
grep -q 'ia_saturation = "1.200000"' "$RETRO/shaders/pegasusg_by_roc_gba/current.glslp"
grep -q 'video_shader_enable = "true"' "$CFG"
grep -q 'keep = "yes"' "$CFG"
grep -q '/custom/overlay.cfg' "$CFG"
grep -q 'pegasusg_by_roc_gba/current.glslp' "$CORE_PRESET"

run_filter original
if grep -q 'ia_saturation' "$RETRO/shaders/pegasusg_by_roc_gba/current.glslp"; then
  echo 'original preset retained calibrated saturation' >&2
  exit 1
fi
printf 'input_overlay = "/temporary/fixed-mode-overlay.cfg"\nvideo_shader = "/temporary/filter.glslp"\n' >>"$CFG"
run_filter original
grep -q '/custom/overlay.cfg' "$CFG"
grep -q '/temporary/fixed-mode-overlay.cfg' "$CFG"

run_filter custom
grep -q 'video_shader_enable = "false"' "$CFG"
grep -q '/custom/first.glslp' "$CFG"
grep -q '/temporary/fixed-mode-overlay.cfg' "$CFG"
grep -q '/custom/core.glslp' "$CORE_PRESET"
grep -q '/custom/content.glslp' "$CONTENT_PRESET"

printf 'video_shader_enable = "true"\nvideo_shader = "/custom/second.glslp"\n' >"$CFG"
run_filter calibrated
run_filter custom
grep -q '/custom/second.glslp' "$CFG"

printf 'video_shader_enable = "true"\nvideo_shader = "/legacy/original.glslp"\n' >"${CFG}.pegasusg_by_roc.original"
printf 'video_shader_enable = "true"\nvideo_shader = "/legacy/injected.glslp"\n' >"$CFG"
rm -f "${CFG}.pegasusg_by_roc.custom" "${CFG}.pegasusg_by_roc.custom.missing" \
  "${CFG}.pegasusg_by_roc.original.migrated"
run_filter calibrated
run_filter custom
grep -q '/legacy/original.glslp' "$CFG"
printf 'video_shader_enable = "true"\nvideo_shader = "/custom/after-migration.glslp"\n' >"$CFG"
run_filter calibrated
run_filter custom
grep -q '/custom/after-migration.glslp' "$CFG"

run_filter calibrated vba_next
test -f "$RETRO/config/VBA Next/VBA Next.cfg"
test -f "$RETRO/config/VBA Next/VBA Next.glslp"
run_filter custom vba_next
test ! -e "$RETRO/config/VBA Next/VBA Next.cfg"
test ! -e "$RETRO/config/VBA Next/VBA Next.glslp"
