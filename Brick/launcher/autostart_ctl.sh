#!/bin/sh
set -eu

APP_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
STATE_DIR="${PEGASUSG_STATE_DIR:-/mnt/data/pegasusg-by-roc}"
TARGET_OVERRIDE="${PEGASUSG_AUTOSTART_TARGET:-}"
BACKUP="$STATE_DIR/autostart.original"
TARGET_RECORD="$STATE_DIR/autostart.target"
BEGIN='# BEGIN PEGASUSG BY ROC AUTOSTART'
END='# END PEGASUSG BY ROC AUTOSTART'

brick_hook_path() {
  printf '%s\n' "${PEGASUSG_BRICK_AUTOSTART_HOOK:-/mnt/SDCARD/System/starts/zz_pegasusg_autostart.sh}"
}

is_brick() {
  [ "${PEGASUSG_DEVICE:-}" = "trimui_brick" ] || [ -d /mnt/SDCARD/System/starts ]
}

brick_enable() {
  hook="$(brick_hook_path)"
  mkdir -p "$(dirname -- "$hook")"
  temporary="$hook.pegasusg.tmp"
  cp "$APP_DIR/autostart_launch.sh" "$temporary"
  chmod 755 "$temporary"
  mv "$temporary" "$hook"
}

brick_disable() {
  rm -f "$(brick_hook_path)"
}

find_target() {
  if [ -n "$TARGET_OVERRIDE" ] && [ -f "$TARGET_OVERRIDE" ]; then
    printf '%s\n' "$TARGET_OVERRIDE"
    return 0
  fi
  for loader in \
    "${PEGASUSG_AUTOSTART_LOADER:-/mnt/vendor/ctrl/loadapp.sh}" \
    /mnt/vendor/ctrl/launcher.sh; do
    [ -f "$loader" ] || continue
    for candidate in \
      /mnt/mod/ctrl/autostart \
      /mnt/mod/ctrl/autostart.sh \
      /mnt/vendor/ctrl/autostart \
      /mnt/vendor/ctrl/autostart.sh; do
      if [ -f "$candidate" ] && grep -Fq "$candidate" "$loader"; then
        printf '%s\n' "$candidate"
        return 0
      fi
    done
  done
  if [ -f "$TARGET_RECORD" ]; then
    IFS= read -r recorded <"$TARGET_RECORD" || recorded=""
    if [ -n "$recorded" ] && [ -f "$recorded" ]; then
      printf '%s\n' "$recorded"
      return 0
    fi
  fi
  for candidate in \
    /mnt/mod/ctrl/autostart \
    /mnt/mod/ctrl/autostart.sh \
    /mnt/vendor/ctrl/autostart \
    /mnt/vendor/ctrl/autostart.sh; do
    if [ -f "$candidate" ]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done
  return 1
}

install_hook() {
  TARGET="$(find_target)" || { echo "autostart target missing" >&2; return 1; }
  [ -f "$TARGET" ] || { echo "autostart target missing" >&2; return 1; }
  mkdir -p "$STATE_DIR"
  [ -f "$BACKUP" ] || cp -p "$TARGET" "$BACKUP"
  cp "$APP_DIR/autostart_launch.sh" "$STATE_DIR/autostart_launch.sh"
  chmod 755 "$STATE_DIR/autostart_launch.sh"
  printf '%s\n' "$APP_DIR" >"$STATE_DIR/app.path"
  printf '%s\n' "$TARGET" >"$TARGET_RECORD"
  last_exit="$(awk '/^exit[[:space:]]+0[[:space:]]*$/ { line=NR } END { print line+0 }' "$TARGET")"
  temporary="$TARGET.pegasusg.tmp"
  awk -v begin="$BEGIN" -v end="$END" -v last_exit="$last_exit" '
    $0 == begin { managed=1; next }
    managed && $0 == end { managed=0; next }
    managed { next }
    !inserted && last_exit > 0 && NR == last_exit {
      print begin
      print "if [ -f /mnt/data/pegasusg-by-roc/autostart.enabled ]; then"
      print "  /mnt/data/pegasusg-by-roc/autostart_launch.sh"
      print "fi"
      print end
      inserted=1
    }
    { print }
    END {
      if (!inserted) {
        print begin
        print "if [ -f /mnt/data/pegasusg-by-roc/autostart.enabled ]; then"
        print "  /mnt/data/pegasusg-by-roc/autostart_launch.sh"
        print "fi"
        print end
      }
    }
  ' "$TARGET" >"$temporary"
  chmod --reference="$TARGET" "$temporary" 2>/dev/null || chmod 755 "$temporary"
  chown --reference="$TARGET" "$temporary" 2>/dev/null || true
  mv "$temporary" "$TARGET"
}

case "${1:-status}" in
  enable)
    if is_brick; then
      brick_enable
    else
      install_hook
      touch "$STATE_DIR/autostart.enabled"
    fi
    ;;
  disable)
    if is_brick; then brick_disable; else rm -f "$STATE_DIR/autostart.enabled"; fi
    ;;
  status)
    if is_brick; then
      [ -f "$(brick_hook_path)" ]
    else
      [ -f "$STATE_DIR/autostart.enabled" ]
    fi
    ;;
  uninstall)
    if is_brick; then
      brick_disable
      exit 0
    fi
    rm -f "$STATE_DIR/autostart.enabled"
    TARGET="$(find_target 2>/dev/null || true)"
    if [ -n "$TARGET" ] && [ -f "$BACKUP" ] && grep -Fq "$BEGIN" "$TARGET"; then
      cp -p "$BACKUP" "$TARGET"
    fi
    ;;
  *)
    echo "usage: $0 enable|disable|status|uninstall" >&2
    exit 2
    ;;
esac
