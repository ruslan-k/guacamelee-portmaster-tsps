#!/bin/bash
# PORTMASTER: guacamelee.zip, Guacamelee.sh

XDG_DATA_HOME=${XDG_DATA_HOME:-$HOME/.local/share}

if [ -d "/opt/system/Tools/PortMaster/" ]; then
  controlfolder="/opt/system/Tools/PortMaster"
elif [ -d "/opt/tools/PortMaster/" ]; then
  controlfolder="/opt/tools/PortMaster"
elif [ -d "$XDG_DATA_HOME/PortMaster/" ]; then
  controlfolder="$XDG_DATA_HOME/PortMaster"
else
  controlfolder="/roms/ports/PortMaster"
fi

source "$controlfolder/control.txt"
[ -f "${controlfolder}/mod_${CFW_NAME}.txt" ] && source "${controlfolder}/mod_${CFW_NAME}.txt"
get_controls

GAMEDIR=/${directory}/ports/guacamelee
cd "$GAMEDIR" || exit 1
mkdir -p "$GAMEDIR/conf" "$GAMEDIR/logs" "$GAMEDIR/gamedata"
LOG="$GAMEDIR/logs/guacamelee.log"
[ -f "$LOG" ] && mv -f "$LOG" "$LOG.1" 2>/dev/null || true
exec >>"$LOG" 2>&1

echo "===== guacamelee start ====="
date 2>/dev/null || true
echo "uname=$(uname -a)"
echo "platform=${PLATFORM:-unset} arch=${PLATFORM_ARCHITECTURE:-unset} cfw=${CFW_NAME:-unset}"

export PORTMASTER_CONTROLFOLDER="$controlfolder"
if [ ! -f "$GAMEDIR/gamedata/game-bin" ]; then
  if ! "$GAMEDIR/setup.sh" "$GAMEDIR"; then
    echo "Game data setup failed"
    sync
    pm_finish
    exit 1
  fi
fi

export XDG_DATA_HOME="$GAMEDIR/conf"
export SDL_GAMECONTROLLERCONFIG="${sdl_controllerconfig:-${SDL_GAMECONTROLLERCONFIG:-}}"
export SDL_JOYSTICK_ALLOW_BACKGROUND_EVENTS=1
export SDL_NO_SIGNAL_HANDLERS=1
export MALLOC_ARENA_MAX=2

BRIDGE=0
case "${GUACAMELEE_TSPS_BRIDGE:-auto}" in
  1|yes|true|on) BRIDGE=1 ;;
  0|no|false|off) BRIDGE=0 ;;
  auto)
    if [ "${PLATFORM:-}" = "SmartProS" ] || \
       { [ "$(uname -m 2>/dev/null)" = "aarch64" ] && [ -d /mnt/SDCARD/spruce ]; }; then
      BRIDGE=1
    fi
    ;;
esac

PRES=0
cleanup() {
  if [ "$PRES" -ne 0 ]; then
    kill "$PRES" 2>/dev/null || true
    wait "$PRES" 2>/dev/null || true
    PRES=0
  fi
  rm -f /tmp/nfsmw.present.ready /tmp/tsp-glbridge.sock /tmp/tspgl-xport /tmp/nfsmw.frame
}
trap cleanup EXIT INT TERM

if [ "$BRIDGE" -eq 1 ]; then
  echo "backend=tsps-32to64-gles-bridge"
  SYS="$GAMEDIR/armhf"
  LD="$SYS/lib/ld-linux-armhf.so.3"
  GLBRIDGE="$GAMEDIR/glbridge"
  PRESENTER="$GAMEDIR/guacamelee_present"
  if [ ! -x "$PRESENTER" ] || [ ! -f "$LD" ] || [ ! -f "$GLBRIDGE/libEGL.so.1" ]; then
    echo "TSPS bridge files are incomplete"
    exit 2
  fi

  export TSPGL_WIDTH="${GUACAMELEE_WIDTH:-640}"
  export TSPGL_HEIGHT="${GUACAMELEE_HEIGHT:-480}"
  export TSPGL_PRESENT="${GUACAMELEE_PRESENT:-letterbox}"
  rm -f /tmp/nfsmw.present.ready /tmp/tsp-glbridge.sock /tmp/tspgl-xport /tmp/nfsmw.frame

  (
    unset LD_PRELOAD
    unset LIBGL_ALWAYS_SOFTWARE GALLIUM_DRIVER MESA_LOADER_DRIVER_OVERRIDE
    unset LIBGL_DRIVERS_PATH __EGL_VENDOR_LIBRARY_FILENAMES EGL_PLATFORM
    export LD_LIBRARY_PATH="/usr/trimui/lib:/usr/lib:/lib:/mnt/SDCARD/spruce/flip/lib"
    export SDL_VIDEO_GL_DRIVER=libGLESv2.so
    export SDL_OPENGL_ES_DRIVER=1
    export XDG_RUNTIME_DIR=/tmp
    export TMPDIR=/tmp
    exec "$PRESENTER"
  ) &
  PRES=$!
  n=0
  while [ "$n" -lt 15 ]; do
    [ -f /tmp/nfsmw.present.ready ] && break
    kill -0 "$PRES" 2>/dev/null || break
    n=$((n + 1))
    sleep 1
  done
  echo "present_ready_wait=$n pid=$PRES"
  if [ ! -f /tmp/nfsmw.present.ready ]; then
    echo "TSPS presenter failed to become ready"
    exit 3
  fi

  export PORT_32BIT=Y
  export LD_LIBRARY_PATH="$GLBRIDGE:$GAMEDIR/box86/native:$SYS/lib/arm-linux-gnueabihf:$SYS/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
  export BOX86_LD_LIBRARY_PATH="$GAMEDIR/box86/x86:$GAMEDIR/gamedata/lib32:$GAMEDIR/libs/x86"
  export SDL_VIDEO_GL_DRIVER="$GAMEDIR/gl4es/libGL.so.1"
  export SDL_VIDEO_EGL_DRIVER="$GLBRIDGE/libEGL.so.1"
  export LIBGL_GLES="$GLBRIDGE/libGLESv2.so.2"
  export LIBGL_EGL="$GLBRIDGE/libEGL.so.1"
  export LIBGL_ES=2
  export LIBGL_GL=21
  export LIBGL_SHRINK="${GUACAMELEE_LIBGL_SHRINK:-4}"
  export LIBGL_FB="${GUACAMELEE_LIBGL_FB:-1}"
  chmod 0755 "$GAMEDIR/box86/box86" "$GAMEDIR/gamedata/game-bin" "$LD"

  echo "bridge_presenter=$PRESENTER"
  echo "bridge_loader=$LD"
  echo "bridge_dimensions=${TSPGL_WIDTH}x${TSPGL_HEIGHT}"
  "$GPTOKEYB2" "game-bin" -c "$GAMEDIR/guacamelee.ini" &
  pm_platform_helper "$GAMEDIR/box86/box86"
  "$LD" --library-path "$LD_LIBRARY_PATH" "$GAMEDIR/box86/box86" "$GAMEDIR/gamedata/game-bin"
  result=$?
else
  echo "backend=legacy-direct-gles"
  export PORT_32BIT=Y
  export LD_LIBRARY_PATH="$GAMEDIR/box86/native:/usr/lib/arm-linux-gnueabihf:/usr/lib32:$GAMEDIR/libs/x86${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
  export BOX86_LD_LIBRARY_PATH="$GAMEDIR/box86/x86:$GAMEDIR/gamedata/lib32:$GAMEDIR/libs/x86"
  export SDL_VIDEO_GL_DRIVER="$GAMEDIR/gl4es/libGL.so.1"
  export LIBGL_SHRINK=4
  "$GPTOKEYB2" "game-bin" -c "$GAMEDIR/guacamelee.ini" &
  pm_platform_helper "$GAMEDIR/box86/box86"
  "$GAMEDIR/box86/box86" "$GAMEDIR/gamedata/game-bin"
  result=$?
fi

echo "exit_code=$result"
echo "===== guacamelee end ====="
sync
cleanup
trap - EXIT INT TERM
pm_finish
exit "$result"
