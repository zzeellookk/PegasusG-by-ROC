#!/bin/sh
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
TEST_ROOT="$(mktemp -d)"
trap 'rm -rf "$TEST_ROOT"' EXIT HUP INT TERM

APP_DIR="$TEST_ROOT/Apps/PegasusG"
HOOK="$TEST_ROOT/System/starts/zz_pegasusg_autostart.sh"
BIN_DIR="$TEST_ROOT/bin"
mkdir -p "$APP_DIR" "$BIN_DIR"
cp "$ROOT/H700/launcher/autostart_ctl.sh" "$APP_DIR/autostart_ctl.sh"
cp "$ROOT/H700/launcher/autostart_launch.sh" "$APP_DIR/autostart_launch.sh"
printf '#!/bin/sh\nexit 0\n' >"$APP_DIR/launch.sh"
printf '#!/bin/sh\nexit 0\n' >"$APP_DIR/pegasusg_by_roc"
chmod 755 "$APP_DIR"/*.sh "$APP_DIR/pegasusg_by_roc"

PEGASUSG_DEVICE=trimui_brick PEGASUSG_BRICK_AUTOSTART_HOOK="$HOOK" \
  "$APP_DIR/autostart_ctl.sh" enable
[ -x "$HOOK" ]
PEGASUSG_DEVICE=trimui_brick PEGASUSG_BRICK_AUTOSTART_HOOK="$HOOK" \
  "$APP_DIR/autostart_ctl.sh" status

cat >"$BIN_DIR/pgrep" <<'EOF'
#!/bin/sh
[ "${1:-}" = MainUI ]
EOF
cat >"$BIN_DIR/killall" <<'EOF'
#!/bin/sh
printf '%s\n' "$*" >"$PEGASUSG_TEST_KILL_RECORD"
EOF
chmod 755 "$BIN_DIR/pgrep" "$BIN_DIR/killall"

COMMAND="$TEST_ROOT/cmd_to_run.sh"
SUBMITTED="$TEST_ROOT/submitted"
HOST_MSG="$TEST_ROOT/host_msg"
KILL_RECORD="$TEST_ROOT/kill_record"
PEGASUSG_DEVICE=trimui_brick \
PEGASUSG_BRICK_APP_DIR="$APP_DIR" \
PEGASUSG_BRICK_CMD_REQUEST="$COMMAND" \
PEGASUSG_BRICK_SUBMITTED_FLAG="$SUBMITTED" \
PEGASUSG_BRICK_HOST_MSG="$HOST_MSG" \
PEGASUSG_BRICK_AUTOSTART_LOG="$TEST_ROOT/autostart.log" \
PEGASUSG_TEST_KILL_RECORD="$KILL_RECORD" \
PATH="$BIN_DIR:$PATH" "$HOOK"

step=0
while [ ! -f "$COMMAND" ] && [ "$step" -lt 20 ]; do
  sleep 0.05
  step=$((step + 1))
done
[ -x "$COMMAND" ]
[ -f "$SUBMITTED" ]
[ -f "$HOST_MSG" ]
[ "$(cat "$KILL_RECORD")" = "-9 MainUI" ]
grep -Fq "exec \"$APP_DIR/launch.sh\" --autostart" "$COMMAND"

PEGASUSG_DEVICE=trimui_brick PEGASUSG_BRICK_AUTOSTART_HOOK="$HOOK" \
  "$APP_DIR/autostart_ctl.sh" disable
[ ! -e "$HOOK" ]
if PEGASUSG_DEVICE=trimui_brick PEGASUSG_BRICK_AUTOSTART_HOOK="$HOOK" \
    "$APP_DIR/autostart_ctl.sh" status; then
  echo "Brick autostart unexpectedly remained enabled" >&2
  exit 1
fi

echo "Brick autostart tests passed"
