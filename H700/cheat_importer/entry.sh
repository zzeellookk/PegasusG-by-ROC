#!/bin/sh
set -eu

APP_DIR="${0%.sh}"

if [ ! -f "$APP_DIR/install.sh" ]; then
  echo "PegasusG by ROC GBA cheat importer is incomplete: $APP_DIR/install.sh" >&2
  exit 4
fi

exec /bin/sh "$APP_DIR/install.sh"
