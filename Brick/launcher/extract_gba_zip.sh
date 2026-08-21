#!/bin/sh

archive="$1"
output="$2"
list_file="${output}.list"
part_file="${output}.part"

[ -f "$archive" ] || exit 2
command -v unzip >/dev/null 2>&1 || exit 3

cleanup() {
  rm -f "$list_file" "$part_file"
}
trap cleanup 0
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

if ! unzip -Z1 "$archive" > "$list_file" 2>/dev/null; then
  # BusyBox unzip does not implement -Z1. Its listing has four leading
  # columns; rebuild member names from column four onwards.
  unzip -l "$archive" 2>/dev/null | awk '
    $1 ~ /^[0-9]+$/ && NF >= 4 {
      name=$4
      for (i=5; i<=NF; i++) name=name " " $i
      print name
    }
  ' > "$list_file" || exit 4
fi

member="$(awk '
  BEGIN { count=0 }
  {
    lower=tolower($0)
    if (lower ~ /\.(gba|agb)$/ && lower !~ /\/$/) {
      count++
      selected=$0
    }
  }
  END {
    if (count == 1) print selected
    else exit 1
  }
' "$list_file")" || exit 5

# Do not pass a damaged legacy Chinese filename back to unzip. Once the scan
# proves there is exactly one GBA member, select it by its ASCII extension.
extension="${member##*.}"
extension_lower="$(printf '%s' "$extension" | tr '[:upper:]' '[:lower:]')"
case "$extension_lower" in
  gba|agb) ;;
  *) exit 6 ;;
esac

if ! unzip -p "$archive" "*.$extension" > "$part_file" 2>/dev/null; then
  exit 7
fi
[ -s "$part_file" ] || exit 8

size="$(wc -c < "$part_file" | tr -d ' ')"
case "$size" in *[!0-9]*|'') exit 9 ;; esac
[ "$size" -le 134217728 ] || exit 10

mv "$part_file" "$output" || exit 11
printf '[zip] extracted one GBA member (%s bytes)\n' "$size"
