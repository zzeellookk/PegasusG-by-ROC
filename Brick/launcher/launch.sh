#!/bin/sh

APP_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
STATE_DIR="$APP_DIR/data"
REQUEST="$STATE_DIR/launch.request"
LOG_FILE="$STATE_DIR/brick-port.log"
RA_DIR="/mnt/SDCARD/RetroArch"
RA_BIN="$RA_DIR/ra64.trimui"
CORE_DIR="$APP_DIR/cores"
GBA_DIR="/mnt/SDCARD/Emus/GBA"
RUMBLE_CORE_OPTIONS="$STATE_DIR/gpsp-rumble-options.cfg"
RUMBLE_RA_APPEND="$STATE_DIR/gpsp-rumble-append.cfg"

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

core_path() {
  case "$1" in
    gpsp_rumble) printf '%s\n' "$CORE_DIR/gpsp_rumble_libretro.so" ;;
    gpsp) printf '%s\n' "$CORE_DIR/gpsp_libretro.so" ;;
    vbam) printf '%s\n' "$CORE_DIR/vbam_libretro.so" ;;
    vba_next) printf '%s\n' "$CORE_DIR/vba_next_libretro.so" ;;
    *) printf '%s\n' "$CORE_DIR/mgba_libretro.so" ;;
  esac
}

prepare_rumble_config() {
  printf 'gpsp_rumble = "enabled"\n' > "$RUMBLE_CORE_OPTIONS"
  printf 'core_options_path = "%s"\n' "$RUMBLE_CORE_OPTIONS" > "$RUMBLE_RA_APPEND"
  printf '[rumble] forced gpsp_rumble=enabled options=%s\n' \
    "$RUMBLE_CORE_OPTIONS" >> "$LOG_FILE"
}

launch_ra64() {
  rom="$1"
  core_file="$2"
  core_choice="$3"

  [ -x "$RA_BIN" ] || { toast "缺少 64 位 RetroArch"; return 1; }
  [ -f "$core_file" ] || { toast "缺少 64 位核心"; return 1; }

  # 与 Brick 固件的 GBA 启动流程保持相同的性能设置。
  if [ -f /mnt/SDCARD/System/scripts/common_launcher.sh ]; then
    . /mnt/SDCARD/System/scripts/common_launcher.sh
  fi
  [ -x "$GBA_DIR/cpufreq.sh" ] && "$GBA_DIR/cpufreq.sh"
  [ -x "$GBA_DIR/cpuswitch.sh" ] && "$GBA_DIR/cpuswitch.sh"

  printf '[ra64] binary=%s core=%s rom=%s\n' "$RA_BIN" "$core_file" "$rom" >> "$LOG_FILE"
  (
    cd "$RA_DIR" || exit 1
    if [ "$core_choice" = "gpsp_rumble" ]; then
      prepare_rumble_config
      HOME="$RA_DIR/" "$RA_BIN" -v --appendconfig "$RUMBLE_RA_APPEND" \
        -L "$core_file" "$rom"
    else
      HOME="$RA_DIR/" "$RA_BIN" -v -L "$core_file" "$rom"
    fi
  ) >> "$LOG_FILE" 2>&1
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

  selected_core="$(core_path "$core")"
  printf '[launcher] choice=%s core=%s rom=%s\n' "$core" "$selected_core" "$rom" >> "$LOG_FILE"

  # 不在天马中解压 ROM；.gba、.zip 等路径原样交给 RetroArch。
  launch_ra64 "$rom" "$selected_core" "$core"
  launch_rc=$?
  if [ "$launch_rc" -ne 0 ]; then
    printf '[launcher] ra64 failed choice=%s rc=%s\n' "$core" "$launch_rc" >> "$LOG_FILE"
    toast "64 位核心启动失败，请查看日志"
    return 1
  fi
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
