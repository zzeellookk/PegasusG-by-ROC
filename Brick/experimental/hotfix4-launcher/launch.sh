#!/bin/sh

APP_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
STATE_DIR="$APP_DIR/data"
REQUEST="$STATE_DIR/launch.request"
LOG_FILE="$STATE_DIR/brick-port.log"
GBA_DIR="/mnt/SDCARD/Emus/GBA"
GPSP_LAUNCHER="$APP_DIR/tools/launch_gpsp_aarch64.sh"
ZIP_EXTRACTOR="$APP_DIR/tools/extract_gba_zip.sh"

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

  launch_rom="$rom"
  temp_rom=""
  rom_lower="$(printf '%s' "$rom" | tr '[:upper:]' '[:lower:]')"
  case "$core:$rom_lower" in
    gpsp:*.zip|gpsp_rumble:*.zip|vba_next:*.zip)
      temp_dir="$STATE_DIR/tmp-rom"
      temp_rom="$temp_dir/game.gba"
      mkdir -p "$temp_dir"
      [ -x "$ZIP_EXTRACTOR" ] || {
        toast "缺少 ZIP 解压工具"
        return 1
      }
      if ! "$ZIP_EXTRACTOR" "$rom" "$temp_rom" >> "$LOG_FILE" 2>&1; then
        rm -f "$temp_rom"
        rmdir "$temp_dir" 2>/dev/null || true
        toast "ZIP 中未找到唯一的 GBA 游戏"
        return 1
      fi
      launch_rom="$temp_rom"
      ;;
  esac

  case "$core" in
    gpsp)
      launcher="$GPSP_LAUNCHER"
      core_file="$APP_DIR/cores/gpsp_libretro.so"
      ;;
    gpsp_rumble)
      launcher="$GPSP_LAUNCHER"
      core_file="$APP_DIR/cores/gpsp_rumble_libretro.so"
      ;;
    vbam) launcher="$GBA_DIR/vbam.sh" ;;
    vba_next) launcher="$GBA_DIR/vbanext.sh" ;;
    *) launcher="$GBA_DIR/launch.sh" ;;
  esac
  [ -x "$launcher" ] || {
    toast "缺少 GBA 启动器"
    [ -n "$temp_rom" ] && rm -f "$temp_rom"
    return 1
  }

  printf '[launcher] core=%s launcher=%s rom=%s\n' "$core" "$launcher" "$rom" >> "$LOG_FILE"
  case "$core" in
    gpsp|gpsp_rumble)
      "$launcher" "$launch_rom" "$core_file" >> "$LOG_FILE" 2>&1
      ;;
    *)
      "$launcher" "$launch_rom" >> "$LOG_FILE" 2>&1
      ;;
  esac
  rc=$?
  if [ -n "$temp_rom" ]; then
    rm -f "$temp_rom"
    rmdir "$STATE_DIR/tmp-rom" 2>/dev/null || true
  fi
  return "$rc"
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
