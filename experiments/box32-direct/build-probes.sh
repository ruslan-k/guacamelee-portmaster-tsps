#!/bin/sh
set -eu
# Build probe sources with target-compatible toolchains.
# Full linking requires target SDL2/EGL/GLES libraries supplied by TSPS.
OUT=${1:-build-probes}
SYSROOT64=${SYSROOT64:?set SYSROOT64 to an AArch64 target sysroot}
SYSROOT32=${SYSROOT32:?set SYSROOT32 to an i386 target sysroot}
mkdir -p "$OUT"

A64_FLAGS="--sysroot=$SYSROOT64 -O2 -Wall -Wextra -I/usr/include -I/usr/include/aarch64-linux-gnu -I/usr/include/SDL2"
A64_LIBS="-L$SYSROOT64/usr/lib -Wl,-rpath,/usr/lib -Wl,--allow-shlib-undefined -l:libSDL2-2.0.so.0.3200.6 -l:libEGL.so.1.4.0 -l:libGLESv2.so.2.1.0 -l:libmali.so.0.32.0 -ldl -lpthread -lm"
aarch64-linux-gnu-gcc $A64_FLAGS -o "$OUT/native_probe" src/native_probe.c $A64_LIBS
aarch64-linux-gnu-gcc $A64_FLAGS -o "$OUT/native_fbo_probe" src/native_fbo_probe.c $A64_LIBS

# i386 probes use the target SDL2 closure. GL entry points are resolved by
# Box64/BOX64_LIBGL at runtime, so no host libGL is linked here.
for src in i386_probe i386_glx_null_probe; do
  i686-linux-gnu-gcc -m32 -O2 -Wall -Wextra \
    -I/usr/include -I/usr/include/aarch64-linux-gnu -I/usr/include/SDL2 \
    -o "$OUT/$src" "src/$src.c" -L"$SYSROOT32/usr/lib" \
    -Wl,-rpath,/mnt/SDCARD/Data/ports/guacamelee/gamedata/lib32 \
    -l:libSDL2-2.0.so.0 -L/usr/lib/i386-linux-gnu -lGL -ldl -lpthread -lm
done
i686-linux-gnu-gcc -m32 -O2 -Wall -Wextra \
  -o "$OUT/i386_dynamic_probe" src/i386_dynamic_probe.c -ldl

file "$OUT"/*
sha256sum "$OUT"/*
