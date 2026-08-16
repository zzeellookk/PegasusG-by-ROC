#!/bin/sh
set -eu

MODE="${1:-enable}"
APP_DIR="${PEGASUSG_APP_DIR:-$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)}"
RETRO_ROOT="${PEGASUSG_RETRO_ROOT:-/mnt/vendor/deep/retro}"
GLOBAL_CONFIG="${PEGASUSG_GLOBAL_RA_CONFIG:-/.config/retroarch/retroarch.cfg}"
STATE_DIR="${PEGASUSG_STATE_DIR:-/mnt/data/pegasusg-by-roc}/recommended-controls"
ASSET_ROOT="$APP_DIR/assets/recommended_controls"

mkdir -p "$STATE_DIR"

merge_enable() {
  target="$1"
  template="$2"
  name="$3"
  saved="$STATE_DIR/$name.saved"
  [ -f "$template" ] || return 1
  mkdir -p "$(dirname -- "$target")"
  if [ ! -e "$saved" ]; then
    if [ -f "$target" ]; then
      awk -F= '
        NR == FNR {
          key = $1; sub(/^[[:space:]]+/, "", key); sub(/[[:space:]]+$/, "", key)
          if (key != "" && key !~ /^#/) wanted[key] = 1
          next
        }
        {
          key = $1; sub(/^[[:space:]]+/, "", key); sub(/[[:space:]]+$/, "", key)
          if (key in wanted) print
        }
      ' "$template" "$target" >"$saved"
    else
      : >"$saved"
      : >"$STATE_DIR/$name.target_missing"
    fi
  fi
  tmp="$target.pegasusg_by_roc.tmp.$$"
  if [ -f "$target" ]; then
    awk -F= '
      NR == FNR {
        key = $1; sub(/^[[:space:]]+/, "", key); sub(/[[:space:]]+$/, "", key)
        if (key != "" && key !~ /^#/) wanted[key] = 1
        next
      }
      {
        key = $1; sub(/^[[:space:]]+/, "", key); sub(/[[:space:]]+$/, "", key)
        if (!(key in wanted)) print
      }
    ' "$template" "$target" >"$tmp"
  else
    : >"$tmp"
  fi
  cat "$template" >>"$tmp"
  mv -f "$tmp" "$target"
  chmod 644 "$target" 2>/dev/null || true
}

merge_disable() {
  target="$1"
  template="$2"
  name="$3"
  saved="$STATE_DIR/$name.saved"
  [ -e "$saved" ] || return 0
  mkdir -p "$(dirname -- "$target")"
  tmp="$target.pegasusg_by_roc.tmp.$$"
  if [ -f "$target" ]; then
    awk -F= '
      NR == FNR {
        key = $1; sub(/^[[:space:]]+/, "", key); sub(/[[:space:]]+$/, "", key)
        if (key != "" && key !~ /^#/) wanted[key] = 1
        next
      }
      {
        key = $1; sub(/^[[:space:]]+/, "", key); sub(/[[:space:]]+$/, "", key)
        if (!(key in wanted)) print
      }
    ' "$template" "$target" >"$tmp"
  else
    : >"$tmp"
  fi
  cat "$saved" >>"$tmp"
  if [ -e "$STATE_DIR/$name.target_missing" ] && [ ! -s "$tmp" ]; then
    rm -f "$tmp" "$target"
  else
    mv -f "$tmp" "$target"
    chmod 644 "$target" 2>/dev/null || true
  fi
  rm -f "$saved" "$STATE_DIR/$name.target_missing"
}

file_enable() {
  target="$1"
  source="$2"
  name="$3"
  backup="$STATE_DIR/$name.file"
  missing="$STATE_DIR/$name.file_missing"
  [ -f "$source" ] || return 1
  if [ ! -e "$backup" ] && [ ! -e "$missing" ]; then
    if [ -f "$target" ]; then
      cp -p "$target" "$backup"
    else
      : >"$missing"
    fi
  fi
  mkdir -p "$(dirname -- "$target")"
  cp -f "$source" "$target"
  chmod 644 "$target" 2>/dev/null || true
}

file_disable() {
  target="$1"
  name="$2"
  backup="$STATE_DIR/$name.file"
  missing="$STATE_DIR/$name.file_missing"
  if [ -f "$backup" ]; then
    mkdir -p "$(dirname -- "$target")"
    cp -p "$backup" "$target"
  elif [ -e "$missing" ]; then
    rm -f "$target"
  fi
  rm -f "$backup" "$missing"
}

restore_legacy_menu_binding() {
  target="$1"
  name="$2"
  saved="$STATE_DIR/$name.saved"
  marker="$STATE_DIR/$name.menu_migrated"
  [ -e "$marker" ] && return 0
  if [ ! -f "$saved" ]; then
    : >"$marker"
    return 0
  fi

  target_tmp="$target.pegasusg_by_roc.menu_tmp.$$"
  saved_tmp="$saved.pegasusg_by_roc.menu_tmp.$$"
  case "$name" in
    global_*)
      if [ -f "$target" ]; then
        sed \
          -e '/^[[:space:]]*input_menu_toggle_btn[[:space:]]*=/d' \
          -e '/^[[:space:]]*input_menu_toggle_gamepad_combo[[:space:]]*=/d' \
          "$target" >"$target_tmp"
      else
        : >"$target_tmp"
      fi
      sed -n \
        -e '/^[[:space:]]*input_menu_toggle_btn[[:space:]]*=/p' \
        -e '/^[[:space:]]*input_menu_toggle_gamepad_combo[[:space:]]*=/p' \
        "$saved" >>"$target_tmp"
      sed \
        -e '/^[[:space:]]*input_menu_toggle_btn[[:space:]]*=/d' \
        -e '/^[[:space:]]*input_menu_toggle_gamepad_combo[[:space:]]*=/d' \
        "$saved" >"$saved_tmp"
      ;;
    *)
      if [ -f "$target" ]; then
        sed \
          -e '/^[[:space:]]*input_menu_toggle[[:space:]]*=/d' \
          -e '/^[[:space:]]*input_menu_toggle_gamepad_combo[[:space:]]*=/d' \
          "$target" >"$target_tmp"
      else
        : >"$target_tmp"
      fi
      sed -n \
        -e '/^[[:space:]]*input_menu_toggle[[:space:]]*=/p' \
        -e '/^[[:space:]]*input_menu_toggle_gamepad_combo[[:space:]]*=/p' \
        "$saved" >>"$target_tmp"
      sed \
        -e '/^[[:space:]]*input_menu_toggle[[:space:]]*=/d' \
        -e '/^[[:space:]]*input_menu_toggle_gamepad_combo[[:space:]]*=/d' \
        "$saved" >"$saved_tmp"
      ;;
  esac
  mv -f "$target_tmp" "$target"
  mv -f "$saved_tmp" "$saved"
  chmod 644 "$target" "$saved" 2>/dev/null || true
  : >"$marker"
}

apply_cfgs() {
  operation="$1"
  "$operation" "$GLOBAL_CONFIG" "$ASSET_ROOT/global.cfg" global_home
  "$operation" "$RETRO_ROOT/retroarch.cfg" "$ASSET_ROOT/global.cfg" global_vendor
  "$operation" "$RETRO_ROOT/config/mGBA/mGBA.cfg" "$ASSET_ROOT/core/mGBA.cfg" core_mgba
  "$operation" "$RETRO_ROOT/config/gpSP/gpSP.cfg" "$ASSET_ROOT/core/gpSP.cfg" core_gpsp
  "$operation" "$RETRO_ROOT/config/VBA-M/VBA-M.cfg" "$ASSET_ROOT/core/VBA-M.cfg" core_vbam
}

case "$MODE" in
  enable)
    restore_legacy_menu_binding "$GLOBAL_CONFIG" global_home
    restore_legacy_menu_binding "$RETRO_ROOT/retroarch.cfg" global_vendor
    restore_legacy_menu_binding "$RETRO_ROOT/config/mGBA/mGBA.cfg" core_mgba
    restore_legacy_menu_binding "$RETRO_ROOT/config/gpSP/gpSP.cfg" core_gpsp
    restore_legacy_menu_binding "$RETRO_ROOT/config/VBA-M/VBA-M.cfg" core_vbam
    apply_cfgs merge_enable
    file_enable "$RETRO_ROOT/remaps/mGBA/mGBA.rmp" "$ASSET_ROOT/remaps/mGBA.rmp" remap_mgba
    file_enable "$RETRO_ROOT/remaps/gpSP/gpSP.rmp" "$ASSET_ROOT/remaps/gpSP.rmp" remap_gpsp
    file_enable "$RETRO_ROOT/remaps/VBA-M/VBA-M.rmp" "$ASSET_ROOT/remaps/VBA-M.rmp" remap_vbam
    ;;
  disable)
    apply_cfgs merge_disable
    file_disable "$RETRO_ROOT/remaps/mGBA/mGBA.rmp" remap_mgba
    file_disable "$RETRO_ROOT/remaps/gpSP/gpSP.rmp" remap_gpsp
    file_disable "$RETRO_ROOT/remaps/VBA-M/VBA-M.rmp" remap_vbam
    ;;
  *)
    echo "usage: $0 enable|disable" >&2
    exit 2
    ;;
esac
