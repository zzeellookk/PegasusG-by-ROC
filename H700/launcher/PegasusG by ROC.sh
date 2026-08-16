#!/bin/sh
set -eu

SELF_DIR="$(cd "$(dirname "$0")" && pwd)"
APP_DIR="$SELF_DIR/PegasusG by ROC"

if [ ! -x "$APP_DIR/launch.sh" ]; then
  echo "PegasusG by ROC launcher missing: $APP_DIR/launch.sh" >&2
  exit 4
fi

exec "$APP_DIR/launch.sh" "$@"
