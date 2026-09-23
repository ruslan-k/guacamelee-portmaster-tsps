#!/usr/bin/env bash
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
COMPAT="$ROOT/portmaster/guacamelee/compat"
OUT="$COMPAT/libgua_sdl_mode_input_fix.so"
ZIG=${ZIG:-zig}
CC_X86=${CC_X86:-i686-linux-gnu-gcc}
if command -v "$ZIG" >/dev/null 2>&1; then
  "$ZIG" cc -target i386-linux-gnu -shared -fPIC -O2 -Wall -Wextra -Werror \
    -Wno-unused-parameter -Wl,-soname,libgua_sdl_mode_input_fix.so \
    -o "$OUT" "$COMPAT/guac_sdl_mode_fix.c" \
    "$COMPAT/guac_sdl_joystick_init.c" -ldl
elif command -v "$CC_X86" >/dev/null 2>&1; then
  "$CC_X86" -m32 -shared -fPIC -O2 -Wall -Wextra -Werror \
    -Wno-unused-parameter -Wl,-soname,libgua_sdl_mode_input_fix.so \
    -o "$OUT" "$COMPAT/guac_sdl_mode_fix.c" \
    "$COMPAT/guac_sdl_joystick_init.c" -ldl
else
  if ! command -v podman >/dev/null 2>&1 || ! podman image exists localhost/guacamelee-box32-toolchain:64; then
    echo "No Zig, i386 cross compiler, or local Box86 toolchain image found" >&2
    exit 1
  fi
  podman run --rm --platform linux/amd64 --security-opt label=disable \
    -v "$ROOT:/src" -w /src localhost/guacamelee-box32-toolchain:64 \
    bash -lc 'i686-linux-gnu-gcc -shared -fPIC -O2 -Wall -Wextra -Werror -Wno-unused-parameter -Wl,-soname,libgua_sdl_mode_input_fix.so -o portmaster/guacamelee/compat/libgua_sdl_mode_input_fix.so portmaster/guacamelee/compat/guac_sdl_mode_fix.c portmaster/guacamelee/compat/guac_sdl_joystick_init.c -ldl'
fi
chmod 0755 "$OUT"
file "$OUT"
