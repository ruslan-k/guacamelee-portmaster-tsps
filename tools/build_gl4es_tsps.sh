#!/usr/bin/env bash
set -euo pipefail

ROOT=${1:-"$(cd "$(dirname "$0")/.." && pwd)"}
SRC=${GL4ES_SRC:-/tmp/gl4es-rbfix}
BUILD=${GL4ES_BUILD:-$SRC/build}
OUT="$ROOT/portmaster/guacamelee/gl4es/libGL.so.1"
PIN=e6448b04e5bfdabffbd650e1ccc53b82cd8c1c5d

if [ ! -d "$SRC/.git" ]; then
  git clone https://github.com/ptitSeb/gl4es.git "$SRC"
fi
actual=$(git -C "$SRC" rev-parse HEAD)
[ "$actual" = "$PIN" ] || { echo "gl4es source must be pinned at $PIN (got $actual)" >&2; exit 2; }
cmake -S "$SRC" -B "$BUILD" -DNOX11=ON -DGLX_STUBS=ON -DEGL_WRAPPER=ON -DGBM=ON
cmake --build "$BUILD" --target GL -j"${JOBS:-2}"
install -Dm755 "$SRC/lib/libGL.so.1" "$OUT"
file "$OUT"
sha256sum "$OUT"
