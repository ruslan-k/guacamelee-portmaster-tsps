#!/usr/bin/env bash
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
PORT="$ROOT/portmaster"
DIST="$PORT/dist"
ZIG=${ZIG:-zig}
mkdir -p "$DIST"
ZIG="$ZIG" bash "$ROOT/tools/build_tsps_bridge.sh"
ZIG="$ZIG" bash "$ROOT/tools/build_hardext_fix.sh"
ZIG="$ZIG" bash "$ROOT/tools/build_sdl_mode_input_fix.sh"
chmod 0755 "$PORT/Guacamelee.sh" "$PORT/guacamelee/setup.sh"
rm -f "$DIST/guacamelee.zip"
cd "$PORT"
zip -9 -r "$DIST/guacamelee.zip" Guacamelee.sh guacamelee \
  -x 'guacamelee/gamedata/*' \
     'guacamelee/gog_guacamelee_gold_edition_2.0.0.3.sh' \
     'guacamelee/logs/*' 'guacamelee/conf/*'
zip -9 -r "$DIST/guacamelee.zip" guacamelee/gamedata/README.txt >/dev/null
