#!/bin/sh
set -eu

ROOT="${TMPDIR:-/tmp}/pegasusg-splash-mode-test-$$"
APP="$ROOT/app"
STATE="$ROOT/state"
VENDOR="$ROOT/vendor"
BOOT="$ROOT/boot"
SCRIPT="H700/launcher/apply_splash.sh"

cleanup() {
  rm -rf "$ROOT"
}
trap cleanup EXIT INT TERM

mkdir -p "$APP/assets/splash" "$STATE" "$BOOT" \
  "$VENDOR/res1/boot" "$VENDOR/res3/boot" \
  "$VENDOR/res1/loading" "$VENDOR/res1/shutdown"
cp H700/assets/splash/bootlogo.bmp "$APP/assets/splash/bootlogo.bmp"
cp H700/assets/splash/splash.png "$APP/assets/splash/splash.png"
cp H700/assets/splash/splash.jpg "$APP/assets/splash/splash.jpg"

targets='boot/bootlogo.bmp
vendor/res1/boot/logo.png
vendor/res3/boot/logo.png
vendor/res1/loading/loading.png
vendor/res1/loading/running_zh.png
vendor/res1/loading/running_en.png
vendor/res1/shutdown/goodbye.png'

printf 'theme-a-boot\n' >"$BOOT/bootlogo.bmp"
printf '%s\n' "$targets" | sed '1d' | while IFS= read -r relative; do
  path="$ROOT/$relative"
  printf 'theme-a:%s\n' "$relative" >"$path"
done

run_splash() {
  PEGASUSG_APP_DIR="$APP" PEGASUSG_STATE_DIR="$STATE" \
    PEGASUSG_VENDOR_ROOT="$VENDOR" PEGASUSG_BOOT_ROOT="$BOOT" \
    sh "$SCRIPT" "$1"
}

run_splash enable
test -f "$STATE/pegasus-splash.enabled"
cmp "$APP/assets/splash/bootlogo.bmp" "$BOOT/bootlogo.bmp"
for target in \
  "$VENDOR/res1/boot/logo.png" \
  "$VENDOR/res3/boot/logo.png" \
  "$VENDOR/res1/loading/loading.png" \
  "$VENDOR/res1/loading/running_zh.png" \
  "$VENDOR/res1/loading/running_en.png" \
  "$VENDOR/res1/shutdown/goodbye.png" \
  "$VENDOR/res1/shutdown/lowpower.png"; do
  cmp "$APP/assets/splash/splash.png" "$target"
done

printf 'temporary-change\n' >"$VENDOR/res1/loading/loading.png"
run_splash enable
cmp "$APP/assets/splash/splash.png" "$VENDOR/res1/loading/loading.png"

run_splash disable
test ! -e "$STATE/pegasus-splash.enabled"
grep -q '^theme-a-boot$' "$BOOT/bootlogo.bmp"
printf '%s\n' "$targets" | sed '1d' | while IFS= read -r relative; do
  grep -q "^theme-a:$relative$" "$ROOT/$relative"
done
test ! -e "$VENDOR/res1/shutdown/lowpower.png"

printf 'theme-b-boot\n' >"$BOOT/bootlogo.bmp"
printf 'theme-b-loading\n' >"$VENDOR/res1/loading/loading.png"
run_splash enable
run_splash disable
grep -q '^theme-b-boot$' "$BOOT/bootlogo.bmp"
grep -q '^theme-b-loading$' "$VENDOR/res1/loading/loading.png"
