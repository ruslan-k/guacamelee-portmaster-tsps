#!/bin/sh
set -eu
# Reproducible diagnostic/clean-candidate gl4es build for this experiment.
# Inputs are explicit so the source revision and patch series are auditable.
SRC=${GL4ES_SRC:-/tmp/gl4es-src}
BUILD=${GL4ES_BUILD:-/mnt/external/Hermes/gl4es-fbo-build}
IMAGE=${GL4ES_IMAGE:-guacamelee-box32-toolchain:64}
SERIES=${GL4ES_SERIES:-experiments/box32-direct/patches/clean-series.txt}
case "$SRC" in /*) ;; *) printf '%s\n' 'GL4ES_SRC must be absolute' >&2; exit 2;; esac
[ -d "$SRC" ] || { printf 'missing source: %s\n' "$SRC" >&2; exit 2; }
mkdir -p "$BUILD"
if [ "${GL4ES_APPLY_SERIES:-0}" = 1 ]; then
  while IFS= read -r p; do
    [ -z "$p" ] && continue
    case "$p" in \#*) continue;; esac
    git -C "$SRC" apply --check "$p"
    git -C "$SRC" apply "$p"
  done < "$SERIES"
fi
podman run --rm --platform linux/amd64 --security-opt label=disable \
  -v "$SRC:/src" -v "$BUILD:/build" "$IMAGE" \
  sh -lc 'cmake --build /build -j2'
printf 'libGL: '; sha256sum "$SRC/lib/libGL.so.1"
printf 'libEGL: '; sha256sum "$SRC/lib/libEGL.so.1"
file "$SRC/lib/libGL.so.1" "$SRC/lib/libEGL.so.1"
readelf --version-info "$SRC/lib/libGL.so.1" | grep -o 'GLIBC_[0-9.]*' | sort -Vu || true
