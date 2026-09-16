#!/bin/sh
set -eu
# Build reproducible game-scoped i386 compatibility shims.
OUT=${1:-build}
mkdir -p "$OUT"

i686-linux-gnu-gcc -m32 -shared -fPIC -nostdlib \
  -Wl,-soname,libbox32_schedshim.so \
  -o "$OUT/libbox32_schedshim.so" src/schedshim.c

i686-linux-gnu-gcc -m32 -shared -fPIC -nostdlib \
  -Wl,-soname,libbox32_affinityshim.so \
  -o "$OUT/libbox32_affinityshim.so" src/affinityshim.c

file "$OUT/libbox32_schedshim.so" "$OUT/libbox32_affinityshim.so"
readelf -h "$OUT/libbox32_schedshim.so" "$OUT/libbox32_affinityshim.so" | grep -E 'Class|Machine'
readelf -Ws "$OUT/libbox32_schedshim.so" | grep -E 'pthread_setschedprio$'
readelf -Ws "$OUT/libbox32_affinityshim.so" | grep -E 'pthread_attr_setaffinity_np$'
sha256sum "$OUT/libbox32_schedshim.so" "$OUT/libbox32_affinityshim.so"
