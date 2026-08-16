#!/bin/sh
set -u

STATE_DIR="${PEGASUSG_STATE_DIR:-/mnt/data/pegasusg-by-roc}"
WAIT_STEPS="${PEGASUSG_AUTOSTART_WAIT_STEPS:-80}"
WAIT_INTERVAL="${PEGASUSG_AUTOSTART_WAIT_INTERVAL:-0.25}"
[ -f "$STATE_DIR/autostart.enabled" ] || exit 0

app_dir=""
if [ -f "$STATE_DIR/app.path" ]; then
  IFS= read -r app_dir <"$STATE_DIR/app.path" || app_dir=""
fi
step=0
while [ "$step" -lt "$WAIT_STEPS" ]; do
  for candidate in \
    "$app_dir" \
    "/mnt/mmc/Roms/APPS/PegasusG by ROC" \
    "/mnt/sdcard/Roms/APPS/PegasusG by ROC"; do
    if [ -n "$candidate" ] && [ -x "$candidate/launch.sh" ] &&
        [ -x "$candidate/pegasusg_by_roc" ]; then
      exec "$candidate/launch.sh" --autostart
    fi
  done
  step=$((step + 1))
  sleep "$WAIT_INTERVAL"
done
exit 0
