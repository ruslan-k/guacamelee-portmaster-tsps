#!/usr/bin/env bash
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
SRC="$ROOT/compat/hardext_fix.c"
OUT="$ROOT/portmaster/guacamelee/compat/libgua_hardext_fix.so"
ZIG=${ZIG:-zig}

mkdir -p "$(dirname -- "$OUT")"
"$ZIG" cc -target arm-linux-gnueabihf -mcpu=cortex_a7 \
  -shared -fPIC -O2 -Wall -Wextra -fvisibility=hidden \
  -Wl,-soname,libgua_hardext_fix.so \
  -o "$OUT" "$SRC" -ldl
chmod 0755 "$OUT"
file "$OUT"
