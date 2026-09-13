#!/bin/sh
set -eu

GAMEDIR=${1:-$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)}
INSTALLER="$GAMEDIR/gog_guacamelee_gold_edition_2.0.0.3.sh"
GAME="$GAMEDIR/gamedata"
CONTROL=${PORTMASTER_CONTROLFOLDER:-}

if [ -f "$GAME/game-bin" ]; then
    echo "Guacamelee game data already extracted"
    exit 0
fi

if [ -z "$CONTROL" ]; then
    echo "PORTMASTER_CONTROLFOLDER is required for first-run extraction" >&2
    exit 2
fi

SEVEN="$CONTROL/7zzs.${DEVICE_ARCH:-aarch64}"
if [ ! -x "$SEVEN" ]; then
    echo "Missing PortMaster extractor: $SEVEN" >&2
    exit 3
fi
if [ ! -f "$INSTALLER" ]; then
    echo "Copy gog_guacamelee_gold_edition_2.0.0.3.sh into $GAMEDIR" >&2
    exit 4
fi

STAGE="$GAMEDIR/.guacamelee-extract"
rm -rf "$STAGE"
mkdir -p "$STAGE"
trap 'rm -rf "$STAGE"' EXIT INT TERM

# 7zzs reads the installer as an archive. It does not execute startmojo.sh.
"$SEVEN" x -tzip -y -aoa "$INSTALLER" -o"$STAGE" >/dev/null
if [ ! -f "$STAGE/data/noarch/game/game-bin" ]; then
    echo "GOG archive did not contain data/noarch/game/game-bin" >&2
    exit 5
fi

mkdir -p "$GAME"
for item in "$STAGE"/data/noarch/game/*; do
    [ -e "$item" ] || continue
    mv "$item" "$GAME/"
done
chmod 0755 "$GAME/game-bin"
[ -f "$GAME/lib32/libSDL2-2.0.so.0" ] || {
    echo "Extracted data is missing lib32/libSDL2-2.0.so.0" >&2
    exit 6
}
[ -f "$GAME/resources.dat" ] || {
    echo "Extracted data is missing resources.dat" >&2
    exit 7
}

echo "Guacamelee game data extraction complete"
