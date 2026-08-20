#!/bin/sh
set -u

if [ "${PEGASUSG_DEVICE:-}" = "trimui_brick" ] || [ -d /mnt/SDCARD/System/starts ]; then
  # System/starts scripts run before MainUI. Return immediately and let a
  # background worker submit PegasusG through the stock cmd_to_run channel.
  (
    submitted="${PEGASUSG_BRICK_SUBMITTED_FLAG:-/tmp/pegasusg_autostart_submitted}"
    command_path="${PEGASUSG_BRICK_CMD_REQUEST:-/tmp/cmd_to_run.sh}"
    host_message="${PEGASUSG_BRICK_HOST_MSG:-/tmp/host_msg}"
    step=0
    while [ "$step" -lt "${PEGASUSG_BRICK_AUTOSTART_WAIT_STEPS:-120}" ]; do
      if pgrep MainUI >/dev/null 2>&1; then
        app_dir="${PEGASUSG_BRICK_APP_DIR:-/mnt/SDCARD/Apps/PegasusG}"
        if [ -x "$app_dir/launch.sh" ] && [ -x "$app_dir/pegasusg_by_roc" ] &&
           [ ! -e "$submitted" ]; then
          touch "$submitted"
          request="$command_path.pegasusg"
          printf '#!/bin/sh\nexec "%s/launch.sh" --autostart\n' "$app_dir" >"$request"
          chmod 755 "$request"
          mv "$request" "$command_path"
          printf 'PegasusG autostart\n' >"$host_message"
          # Brick MainUI ignores SIGTERM. Its stock runtrimui.sh accepts
          # exit code 137, so retain the already verified SIGKILL handoff.
          killall -9 MainUI 2>/dev/null || true
        fi
        exit 0
      fi
      step=$((step + 1))
      sleep "${PEGASUSG_BRICK_AUTOSTART_WAIT_INTERVAL:-0.25}"
    done
    exit 0
  ) >>"${PEGASUSG_BRICK_AUTOSTART_LOG:-/tmp/pegasusg-autostart.log}" 2>&1 &
  exit 0
fi

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
