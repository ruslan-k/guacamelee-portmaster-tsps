#!/bin/sh
# PORTMASTER: guacamelee-box32-direct.zip, Guacamelee-box32-direct.sh
# Do not enable nounset: PortMaster sources files with optional variables.

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

GAMEDIR=
for candidate in \
  /mnt/SDCARD/Data/ports/guacamelee \
  "${directory:+${directory%/}/guacamelee}" \
  "${directory:+/${directory#/}/ports/guacamelee}" \
  /roms/ports/guacamelee /sdcard/ports/guacamelee; do
  if [ -d "$candidate" ]; then GAMEDIR="$candidate"; break; fi
done
[ -n "$GAMEDIR" ] || { echo "Guacamelee data directory not found" >&2; exit 1; }
cd "$GAMEDIR" || exit 1
mkdir -p "$GAMEDIR/logs"
LOG="$GAMEDIR/logs/box32-direct.log"
[ -f "$LOG" ] && mv -f "$LOG" "$LOG.1" 2>/dev/null || true
exec >>"$LOG" 2>&1

echo "===== guacamelee box32-direct start ====="
date 2>/dev/null || true
echo "uname=$(uname -a)"
echo "platform=${PLATFORM:-unset} arch=${PLATFORM_ARCHITECTURE:-unset} cfw=${CFW_NAME:-unset}"
echo "backend=box64-box32-aarch64-gl4es-direct"

BOX64="$GAMEDIR/box32-direct/box64-box32-aarch64"
GL4ES="$GAMEDIR/box32-direct/libGL.so.1"
SCHEDSHIM="$GAMEDIR/box32-direct/libbox32_schedshim.so"
X11STUB="$GAMEDIR/box32-direct/libX11.so.6"
GAME="$GAMEDIR/box32-direct/game-bin"
[ -x "$BOX64" ] || { echo "missing Box64/Box32: $BOX64"; exit 2; }
[ -f "$GL4ES" ] || { echo "missing AArch64 gl4es: $GL4ES"; exit 3; }
[ -f "$SCHEDSHIM" ] || { echo "missing Box32 sched shim: $SCHEDSHIM"; exit 4; }
[ -f "$X11STUB" ] || { echo "missing direct X11 dlopen stub: $X11STUB"; exit 5; }
[ -f "$GAME" ] || { echo "missing game-bin: $GAME"; exit 6; }

NATIVE_EGL=/usr/trimui/lib/libEGL.so.1
NATIVE_GLES=/usr/trimui/lib/libGLESv2.so.2
if [ ! -e "$NATIVE_EGL" ]; then NATIVE_EGL=/usr/lib/libEGL.so.1; fi
if [ ! -e "$NATIVE_GLES" ]; then NATIVE_GLES=/usr/lib/libGLESv2.so.2; fi

echo "box64=$BOX64"
echo "game=$GAME"
echo "BOX64_LIBGL=$GL4ES"
echo "BOX64_LD_PRELOAD=$SCHEDSHIM"
echo "native_egl=$NATIVE_EGL"
echo "native_gles=$NATIVE_GLES"
echo "pipeline=NO_ARMHF NO_GLBRIDGE NO_PRESENTER"

export PORTMASTER_CONTROLFOLDER="$controlfolder"
export XDG_DATA_HOME="$GAMEDIR/conf"
export SDL_GAMECONTROLLERCONFIG="${sdl_controllerconfig:-${SDL_GAMECONTROLLERCONFIG:-}}"
export SDL_JOYSTICK_ALLOW_BACKGROUND_EVENTS=1
export SDL_NO_SIGNAL_HANDLERS=1
export MALLOC_ARENA_MAX=2
export PORT_32BIT=Y
export XDG_RUNTIME_DIR=/tmp
export TMPDIR=/tmp
export SDL_VIDEODRIVER="${GUACAMELEE_SDL_VIDEODRIVER:-KMSDRM}"
export SDL_KMSDRM_REQUIRE_DRM_MASTER=0
export SDL_VIDEO_GL_DRIVER="$GL4ES"
export SDL_VIDEO_EGL_DRIVER="$NATIVE_EGL"
export BOX64_LIBGL="$GL4ES"
export BOX64_LD_PRELOAD="$SCHEDSHIM"
export BOX64_RESERVE_HIGH=0
export BOX64_MMAP32=1
export BOX32_PERSONA32BITS=1
export BOX64_PREFER_WRAPPED=1
export BOX64_LOG=2
export BOX64_DLSYM_ERROR=1
export BOX64_SHOWSEGV=1
export BOX64_SHOWBT=1
export BOX64_ROLLING_LOG=1
export BOX64_WRAP_EGL=1
export BOX64_LD_LIBRARY_PATH="$GAMEDIR/box86/x86:$GAMEDIR/gamedata/lib32:$GAMEDIR/libs/x86"
export LD_LIBRARY_PATH="$GAMEDIR/box32-direct:/usr/lib:/lib"
export LIBGL_GLES="$NATIVE_GLES"
export LIBGL_EGL="$NATIVE_EGL"
export LIBGL_ES=2
export LIBGL_GL=21
export LIBGL_NOTEST=1
export LIBGL_LOGSHADERERROR=1
export LIBGL_SILENTSTUB=0
export LIBGL_FPS=1

chmod 0755 "$BOX64" "$GAME"
$GPTOKEYB2 "game-bin" -c "$GAMEDIR/guacamelee.ini" &
GPTOKEYB_PID=$!
# Box64 direct A/B intentionally omits pm_platform_helper: the guest sets its own thread priority.
"$BOX64" "$GAME"
result=$?
kill "$GPTOKEYB_PID" 2>/dev/null || true
wait "$GPTOKEYB_PID" 2>/dev/null || true

echo "exit_code=$result"
echo "===== guacamelee box32-direct end ====="
sync
pm_finish
exit "$result"
