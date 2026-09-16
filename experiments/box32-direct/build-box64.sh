#!/bin/sh
set -eu
# Apply src/box64-box32-lowptr.patch to the pinned upstream Box64 checkout,
# then build with an Ubuntu 20.04/glibc 2.31 cross-toolchain.
BOX64_SRC=${BOX64_SRC:?set BOX64_SRC to a Box64 checkout}
BUILD=${BUILD:-box64-build}
mkdir -p "$BUILD"
git -C "$BOX64_SRC" apply --check "$PWD/src/box64-box32-lowptr.patch"
git -C "$BOX64_SRC" apply "$PWD/src/box64-box32-lowptr.patch"
cmake -S "$BOX64_SRC" -B "$BUILD" \
  -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
  -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
  -DARM_DYNAREC=ON -DBOX32=ON -DBOX32_BINFMT=OFF \
  -DCMAKE_BUILD_TYPE=Release -DSTATICBUILD=OFF
cmake --build "$BUILD" -j2
file "$BUILD/box64"
readelf --version-info "$BUILD/box64" | grep -o 'Name: GLIBC_[0-9.]*' | sort -Vu
sha256sum "$BUILD/box64"
