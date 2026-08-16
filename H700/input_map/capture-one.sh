#!/bin/sh

device="${1:-/dev/input/event1}"
label="${2:-UNKNOWN}"
log="${3:-/mnt/data/pegasusg-by-roc/logs/input-map.txt}"

if [ ! -r "$device" ]; then
  echo "ERROR=device-not-readable:$device"
  exit 1
fi

mkdir -p "$(dirname "$log")"
echo "READY=$label"

event_line="$({
  timeout 30 evtest --grab "$device" 2>/dev/null || true
} | awk '
  /Event: time/ && /type 1 \(EV_KEY\)/ && /value 1$/ { print; exit }
  /Event: time/ && /type 3 \(EV_ABS\)/ && $0 !~ /value 0$/ { print; exit }
')"

if [ -z "$event_line" ]; then
  echo "TIMEOUT=$label"
  exit 2
fi

result="$label|$device|$event_line"
printf '%s\n' "$result" >>"$log"
sync
echo "CAPTURED=$result"
