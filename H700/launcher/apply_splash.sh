#!/bin/sh
set -u

ACTION="${1:-status}"
APP_DIR="${PEGASUSG_APP_DIR:-$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)}"
STATE_DIR="${PEGASUSG_STATE_DIR:-/mnt/data/pegasusg-by-roc}"
VENDOR_ROOT="${PEGASUSG_VENDOR_ROOT:-/mnt/vendor}"
BOOT_ROOT="${PEGASUSG_BOOT_ROOT:-/mnt/boot}"
BOOT_DEVICE="${PEGASUSG_BOOT_DEVICE:-/dev/mmcblk0p2}"
ASSET_DIR="$APP_DIR/assets/splash"
BACKUP_DIR="$STATE_DIR/splash-backup"
ACTIVE_MARKER="$STATE_DIR/pegasus-splash.enabled"
LOG_FILE="$STATE_DIR/logs/PegasusG-by-ROC.log"
MOUNTED_BY_US=0

mkdir -p "$STATE_DIR/logs"

log_line() {
  printf '%s\n' "$*" >>"$LOG_FILE"
}

mount_boot() {
  mkdir -p "$BOOT_ROOT" || return 1
  if [ -n "${PEGASUSG_BOOT_ROOT:-}" ]; then
    return 0
  fi
  if grep -qs "[[:space:]]$BOOT_ROOT[[:space:]]" /proc/mounts; then
    return 0
  fi
  mount -t vfat -o rw,utf8,noatime "$BOOT_DEVICE" "$BOOT_ROOT" || return 1
  MOUNTED_BY_US=1
}

unmount_boot() {
  if [ "$MOUNTED_BY_US" -eq 1 ]; then
    umount "$BOOT_ROOT" || return 1
    rmdir "$BOOT_ROOT" 2>/dev/null || true
    MOUNTED_BY_US=0
  fi
}

backup_path() {
  target="$1"
  backup="$2"
  missing="${backup}.missing"
  mkdir -p "$(dirname -- "$backup")" || return 1
  if [ -f "$target" ]; then
    cp -p "$target" "$backup" || return 1
  else
    : >"$missing" || return 1
  fi
}

restore_path() {
  target="$1"
  backup="$2"
  missing="${backup}.missing"
  if [ -f "$backup" ]; then
    mkdir -p "$(dirname -- "$target")" || return 1
    temporary="${target}.pegasusg.tmp.$$"
    cp -p "$backup" "$temporary" || { rm -f "$temporary"; return 1; }
    mv -f "$temporary" "$target" || { rm -f "$temporary"; return 1; }
  elif [ -e "$missing" ]; then
    rm -f "$target" || return 1
  else
    return 1
  fi
}

install_path() {
  source="$1"
  target="$2"
  [ -f "$source" ] || return 1
  mkdir -p "$(dirname -- "$target")" || return 1
  temporary="${target}.pegasusg.tmp.$$"
  cp -p "$source" "$temporary" || { rm -f "$temporary"; return 1; }
  mv -f "$temporary" "$target" || { rm -f "$temporary"; return 1; }
}

for_each_target() {
  command="$1"
  "$command" "$BOOT_ROOT/bootlogo.bmp" "$BACKUP_DIR/boot/bootlogo.bmp" "$ASSET_DIR/bootlogo.bmp" || return 1
  "$command" "$VENDOR_ROOT/res1/boot/logo.png" "$BACKUP_DIR/vendor/res1/boot/logo.png" "$ASSET_DIR/splash.png" || return 1
  "$command" "$VENDOR_ROOT/res3/boot/logo.png" "$BACKUP_DIR/vendor/res3/boot/logo.png" "$ASSET_DIR/splash.png" || return 1
  "$command" "$VENDOR_ROOT/res1/loading/loading.png" "$BACKUP_DIR/vendor/res1/loading/loading.png" "$ASSET_DIR/splash.png" || return 1
  "$command" "$VENDOR_ROOT/res1/loading/running_zh.png" "$BACKUP_DIR/vendor/res1/loading/running_zh.png" "$ASSET_DIR/splash.png" || return 1
  "$command" "$VENDOR_ROOT/res1/loading/running_en.png" "$BACKUP_DIR/vendor/res1/loading/running_en.png" "$ASSET_DIR/splash.png" || return 1
  "$command" "$VENDOR_ROOT/res1/shutdown/goodbye.png" "$BACKUP_DIR/vendor/res1/shutdown/goodbye.png" "$ASSET_DIR/splash.png" || return 1
  "$command" "$VENDOR_ROOT/res1/shutdown/lowpower.png" "$BACKUP_DIR/vendor/res1/shutdown/lowpower.png" "$ASSET_DIR/splash.png" || return 1
}

backup_target() {
  backup_path "$1" "$2"
}

install_target() {
  install_path "$3" "$1"
}

restore_target() {
  restore_path "$1" "$2"
}

enable_splash() {
  [ -f "$ASSET_DIR/bootlogo.bmp" ] && [ -f "$ASSET_DIR/splash.png" ] || {
    log_line '[splash] required assets missing'
    return 1
  }
  mount_boot || { log_line '[splash] failed to mount boot partition'; return 1; }
  if [ ! -f "$ACTIVE_MARKER" ]; then
    rm -rf "$BACKUP_DIR"
    if ! for_each_target backup_target; then
      log_line '[splash] failed to preserve current transition images'
      unmount_boot || true
      return 1
    fi
  fi
  if ! for_each_target install_target; then
    log_line '[splash] failed to install all transition images; restoring backup'
    for_each_target restore_target || true
    rm -f "$ACTIVE_MARKER"
    unmount_boot || true
    return 1
  fi
  : >"$ACTIVE_MARKER"
  sync
  unmount_boot || return 1
  log_line '[splash] PegasusG transition images enabled (8 targets)'
}

disable_splash() {
  if [ ! -f "$ACTIVE_MARKER" ]; then
    return 0
  fi
  mount_boot || { log_line '[splash] failed to mount boot partition for restore'; return 1; }
  if ! for_each_target restore_target; then
    log_line '[splash] failed to restore all transition images'
    unmount_boot || true
    return 1
  fi
  rm -f "$ACTIVE_MARKER"
  rm -rf "$BACKUP_DIR"
  sync
  unmount_boot || return 1
  log_line '[splash] previous transition images restored'
}

case "$ACTION" in
  enable) enable_splash ;;
  disable) disable_splash ;;
  status) [ -f "$ACTIVE_MARKER" ] ;;
  *) echo 'usage: apply_splash.sh enable|disable|status' >&2; exit 2 ;;
esac
