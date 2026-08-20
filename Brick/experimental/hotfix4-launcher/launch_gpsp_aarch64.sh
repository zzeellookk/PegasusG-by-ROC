#!/bin/sh

. /mnt/SDCARD/System/scripts/common_launcher.sh

RA_DIR="/mnt/SDCARD/RetroArch"
EMU_DIR="/mnt/SDCARD/Emus/GBA"
rom_file="$1"
core_file="$2"

[ -f "$core_file" ] || exit 2
[ -f "$rom_file" ] || exit 3

cd "$RA_DIR" || exit 4
"$EMU_DIR/cpufreq.sh"
"$EMU_DIR/cpuswitch.sh"

HOME="$RA_DIR/" "$RA_DIR/ra64.trimui" -v -L "$core_file" "$rom_file"
