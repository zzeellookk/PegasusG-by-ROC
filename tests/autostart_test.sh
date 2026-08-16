#!/bin/sh
set -eu

ROOT="${TMPDIR:-/tmp}/pegasusg-autostart-test-$$"
APP="$ROOT/PegasusG by ROC"
STATE="$ROOT/state"
TARGET="$ROOT/autostart.sh"
CONTROL="H700/launcher/autostart_ctl.sh"
LAUNCHER="H700/launcher/autostart_launch.sh"

cleanup() {
  rm -rf "$ROOT"
}
trap cleanup EXIT INT TERM
mkdir -p "$APP" "$STATE"
printf '#!/bin/sh\nif false; then\n  exit 0\nfi\nprintf before\nexit 0\n' >"$TARGET"
chmod 755 "$TARGET"
cp H700/launcher/autostart_ctl.sh "$APP/autostart_ctl.sh"
cp H700/launcher/autostart_launch.sh "$APP/autostart_launch.sh"
chmod 755 "$APP/autostart_ctl.sh" "$APP/autostart_launch.sh"

PEGASUSG_STATE_DIR="$STATE" PEGASUSG_AUTOSTART_TARGET="$TARGET" \
  "$APP/autostart_ctl.sh" enable
test -f "$STATE/autostart.enabled"
grep -q '^# BEGIN PEGASUSG BY ROC AUTOSTART$' "$TARGET"
grep -n '^# BEGIN PEGASUSG BY ROC AUTOSTART$' "$TARGET" | cut -d: -f1 >"$ROOT/begin.line"
grep -n '^exit 0$' "$TARGET" | tail -n 1 | cut -d: -f1 >"$ROOT/exit.line"
test "$(cat "$ROOT/begin.line")" -lt "$(cat "$ROOT/exit.line")"
test "$(cat "$ROOT/begin.line")" -gt 4

# Enabling again relocates an older managed block instead of leaving duplicates.
PEGASUSG_STATE_DIR="$STATE" PEGASUSG_AUTOSTART_TARGET="$TARGET" \
  "$APP/autostart_ctl.sh" enable
test "$(grep -c '^# BEGIN PEGASUSG BY ROC AUTOSTART$' "$TARGET")" -eq 1

# The boot hook waits for slower card mounting on 34XXSP firmware.
rm -f "$APP/launch.sh" "$APP/pegasusg_by_roc"
(
  sleep 0.15
  printf '#!/bin/sh\nprintf "%s\\n" "$1" >"%s"\n' '%s' "$ROOT/launched.args" >"$APP/launch.sh"
  : >"$APP/pegasusg_by_roc"
  chmod 755 "$APP/launch.sh" "$APP/pegasusg_by_roc"
) &
PEGASUSG_STATE_DIR="$STATE" PEGASUSG_AUTOSTART_WAIT_STEPS=20 \
  PEGASUSG_AUTOSTART_WAIT_INTERVAL=0.05 sh "$LAUNCHER"
grep -q '^--autostart$' "$ROOT/launched.args"

PEGASUSG_STATE_DIR="$STATE" PEGASUSG_AUTOSTART_TARGET="$TARGET" \
  "$APP/autostart_ctl.sh" disable
test ! -e "$STATE/autostart.enabled"
