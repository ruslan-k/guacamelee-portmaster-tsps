#!/usr/bin/env bash
set -euo pipefail
ROOT=${1:-"$(cd "$(dirname "$0")/.." && pwd)"}
IMAGE=${GL4ES_CONTAINER_IMAGE:-docker.io/library/ubuntu:20.04}
NAME=guacamelee-gl4es-spruce-build
podman run --rm --name "$NAME" --security-opt label=disable \
  -e DEBIAN_FRONTEND=noninteractive \
  -v "$ROOT:/src:Z" -w /src "$IMAGE" bash -ceu '
export TZ=UTC
apt-get update
apt-get install -y --no-install-recommends software-properties-common ca-certificates
sed -i "s/^deb http/deb [arch=amd64] http/g" /etc/apt/sources.list
printf "%s\n" \
  "deb [arch=armhf] http://ports.ubuntu.com/ubuntu-ports focal main restricted universe multiverse" \
  "deb [arch=armhf] http://ports.ubuntu.com/ubuntu-ports focal-updates main restricted universe multiverse" \
  "deb [arch=armhf] http://ports.ubuntu.com/ubuntu-ports focal-security main restricted universe multiverse" \
  >/etc/apt/sources.list.d/armhf.list
dpkg --add-architecture armhf
apt-get update
apt-get install -y --no-install-recommends \
  build-essential cmake ninja-build git patch pkg-config file \
  gcc-arm-linux-gnueabihf libc6-dev-armhf-cross \
  libdrm-dev:armhf libgbm-dev:armhf libegl1-mesa-dev:armhf
rm -rf /tmp/gl4es-source /tmp/gl4es-build
mkdir -p /tmp/gl4es-source /tmp/gl4es-build
git clone --filter=blob:none https://github.com/ptitSeb/gl4es.git /tmp/gl4es-source
cd /tmp/gl4es-source
git checkout --detach e6448b04e5bfdabffbd650e1ccc53b82cd8c1c5d
git apply --check /src/patches/gl4es-tsps-safe-profile.patch
git apply /src/patches/gl4es-tsps-safe-profile.patch
cat >/tmp/armhf-toolchain.cmake <<EOF
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)
set(CMAKE_FIND_ROOT_PATH /usr/arm-linux-gnueabihf /usr/lib/arm-linux-gnueabihf)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
EOF
export PKG_CONFIG_LIBDIR=/usr/lib/arm-linux-gnueabihf/pkgconfig:/usr/share/pkgconfig
export PKG_CONFIG_PATH=/usr/lib/arm-linux-gnueabihf/pkgconfig
cmake -S /tmp/gl4es-source -B /tmp/gl4es-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/tmp/armhf-toolchain.cmake \
  -DNOX11=ON -DGLX_STUBS=ON -DEGL_WRAPPER=ON -DGBM=OFF
cmake --build /tmp/gl4es-build --target GL -j2
install -Dm755 /tmp/gl4es-source/lib/libGL.so.1 /src/portmaster/guacamelee/gl4es/libGL.so.1
file /src/portmaster/guacamelee/gl4es/libGL.so.1
readelf --version-info /src/portmaster/guacamelee/gl4es/libGL.so.1 | grep -o "GLIBC_[0-9.]*" | sort -Vu || true
sha256sum /src/portmaster/guacamelee/gl4es/libGL.so.1
'