#!/bin/sh

APP_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
STATE_DIR="$APP_DIR/data"
REQUEST="$STATE_DIR/launch.request"
LOG_FILE="$STATE_DIR/brick-port.log"
GBA_DIR="/mnt/SDCARD/Emus/GBA"

mkdir -p "$STATE_DIR"
cd "$APP_DIR" || exit 1

export HOME="$STATE_DIR/home"
export XDG_CONFIG_HOME="$STATE_DIR/config"
export PEGASUSG_DEVICE="trimui_brick"
export PEGASUSG_APP_DIR="$APP_DIR"
export PEGASUSG_STATE_DIR="$STATE_DIR"
export PEGASUSG_LAUNCH_REQUEST="$REQUEST"
export PEGASUSG_CONTENT_ROOTS="/mnt/SDCARD/Roms/GBA:/mnt/SDCARD/Roms/GBA hack:/mnt/SDCARD/Roms/GBA vib"
export PEGASUSG_FONT="/usr/trimui/res/regular.ttf"
export PEGASUSG_WIDTH="1024"
export PEGASUSG_HEIGHT="768"
export PEGASUSG_NO_VIDEO="1"
export SDL_GAMECONTROLLERCONFIG_FILE="/usr/trimui/gamecontrollerdb.txt"
export LD_LIBRARY_PATH="/usr/trimui/lib:/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

mkdir -p "$HOME" "$XDG_CONFIG_HOME"

toast() {
  message="$1"
  if [ -d /tmp/trimui_osd ]; then
    printf '{ "type":"default", "size":2, "duration":3000, "x":212, "y":5, "align":"center", "message":"%s", "icon":"" }\n' "$message" \
      > /tmp/trimui_osd/osd_toast_msg
  fi
}

launch_game() {
  [ -f "$REQUEST" ] || return 1
  rom="$(sed -n '1p' "$REQUEST")"
  core="$(sed -n '2p' "$REQUEST")"
  : > "$REQUEST"

  case "$rom" in
    /mnt/SDCARD/Roms/GBA/*|/mnt/SDCARD/Roms/GBA\ hack/*|/mnt/SDCARD/Roms/GBA\ vib/*) ;;
    *) toast "已拒绝不安全的游戏路径"; return 1 ;;
  esac
  [ -f "$rom" ] || { toast "游戏文件不存在"; return 1; }

  case "$core" in
    gpsp|gpsp_rumble) launcher="launch_gpsp.sh" ;;
    vbam) launcher="vbam.sh" ;;
    vba_next) launcher="vbanext.sh" ;;
    *) launcher="launch.sh" ;;
  esac
  [ -x "$GBA_DIR/$launcher" ] || {
    toast "缺少 GBA 启动器：$launcher"
    return 1
  }

  printf '[launcher] core=%s launcher=%s rom=%s\n' "$core" "$launcher" "$rom" >> "$LOG_FILE"
  "$GBA_DIR/$launcher" "$rom" >> "$LOG_FILE" 2>&1
}

restore_ui=""
while :; do
  "$APP_DIR/pegasusg_by_roc" $restore_ui >> "$LOG_FILE" 2>&1
  rc=$?
  case "$rc" in
    20)
      launch_game || true
      restore_ui="--restore-ui"
      ;;
    21|22|23|24)
      toast "Brick 适配版已禁用系统电源操作"
      exit 0
      ;;
    *) exit "$rc" ;;
  esac
done
