#!/bin/sh
set -eu

GAMEDIR=${1:-$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)}
INSTALLER="$GAMEDIR/gog_guacamelee_gold_edition_2.0.0.3.sh"
GAME="$GAMEDIR/gamedata"
CONTROL=${PORTMASTER_CONTROLFOLDER:-}

patch_game_affinity() {
    game_bin=$1
    offset=$((0x858da3))
    bytes=$(dd if="$game_bin" bs=1 skip="$offset" count=5 2>/dev/null | od -An -tx1 | tr -d ' \n')
    case "$bytes" in
        e808b07aff)
            if [ ! -f "$game_bin.affinity-original" ]; then
                cp "$game_bin" "$game_bin.affinity-original"
            fi
            printf '\220\220\220\220\220' | dd of="$game_bin" bs=1 seek="$offset" conv=notrunc 2>/dev/null
            chmod 0755 "$game_bin"
            echo "Applied Guacamelee CPU-affinity compatibility patch"
            ;;
        9090909090)
            echo "Guacamelee CPU-affinity compatibility patch already applied"
            ;;
        *)
            echo "Unsupported game-bin affinity bytes at 0x858da3: $bytes" >&2
            return 1
            ;;
    esac
}

if [ -f "$GAME/game-bin" ]; then
    patch_game_affinity "$GAME/game-bin"
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
patch_game_affinity "$GAME/game-bin"
[ -f "$GAME/lib32/libSDL2-2.0.so.0" ] || {
    echo "Extracted data is missing lib32/libSDL2-2.0.so.0" >&2
    exit 6
}
[ -f "$GAME/resources.dat" ] || {
    echo "Extracted data is missing resources.dat" >&2
    exit 7
}
rm -f "$INSTALLER"
echo "Guacamelee game data extraction complete"
