#!/bin/sh
set -eu
# Build reproducible diagnostic shims in a Debian/Ubuntu cross-toolchain.
# Run from this directory with the repository root as the working directory.
OUT=${1:-build}
mkdir -p "$OUT"

i686-linux-gnu-gcc -m32 -shared -fPIC -nostdlib \
  -Wl,-soname,libbox32_schedshim.so \
  -o "$OUT/libbox32_schedshim.so" src/schedshim.c

aarch64-linux-gnu-gcc -shared -fPIC \
  -Wl,-soname,libX11.so.6 \
  -o "$OUT/libX11.so.6" src/x11stub.c

file "$OUT/libbox32_schedshim.so" "$OUT/libX11.so.6"
readelf -h "$OUT/libbox32_schedshim.so" | grep -E 'Class|Machine'
readelf -h "$OUT/libX11.so.6" | grep -E 'Class|Machine'
sha256sum "$OUT/libbox32_schedshim.so" "$OUT/libX11.so.6"
