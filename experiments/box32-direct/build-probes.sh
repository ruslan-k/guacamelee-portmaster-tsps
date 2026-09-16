#!/bin/sh
set -eu
# Build probe sources with target-compatible toolchains.
# Full linking requires target SDL2/EGL/GLES libraries supplied by TSPS.
OUT=${1:-build-probes}; SYSROOT64=${SYSROOT64:?set SYSROOT64 to an AArch64 target sysroot}; SYSROOT32=${SYSROOT32:?set SYSROOT32 to an i386 target sysroot}
mkdir -p "$OUT"
aarch64-linux-gnu-gcc --sysroot="$SYSROOT64" -O2 -Wall -Wextra -o "$OUT/native_probe" src/native_probe.c -I"$SYSROOT64/usr/include" -L"$SYSROOT64/usr/lib" -lSDL2 -lEGL -lGLESv2
# GLXNULL is intentionally a guest-side probe; link against the bundled i386 GL frontend.
i686-linux-gnu-gcc -m32 -O2 -Wall -Wextra -o "$OUT/i386_probe" src/i386_probe.c -I"$SYSROOT32/usr/include" -L"$SYSROOT32/usr/lib" -lSDL2 -lGL
i686-linux-gnu-gcc -m32 -O2 -Wall -Wextra -o "$OUT/i386_glx_null_probe" src/i386_glx_null_probe.c -I"$SYSROOT32/usr/include" -L"$SYSROOT32/usr/lib" -lSDL2 -lGL
file "$OUT"/*
sha256sum "$OUT"/*
