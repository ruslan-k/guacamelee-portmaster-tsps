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

GAMEDIR=
for candidate in \
  "${directory:+${directory%/}/guacamelee}" \
  "${directory:+/${directory#/}/ports/guacamelee}" \
  /mnt/SDCARD/Data/ports/guacamelee \
  /roms/ports/guacamelee /sdcard/ports/guacamelee; do
  if [ -d "$candidate" ]; then
    GAMEDIR="$candidate"
    break
  fi
done
[ -n "$GAMEDIR" ] || { echo "Guacamelee data directory not found" >&2; exit 1; }
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

  export GUACAMELEE_WIDTH="${GUACAMELEE_WIDTH:-1024}"
  export GUACAMELEE_HEIGHT="${GUACAMELEE_HEIGHT:-768}"
  export TSPGL_WIDTH="$GUACAMELEE_WIDTH"
  export TSPGL_HEIGHT="$GUACAMELEE_HEIGHT"
  export TSPGL_PRESENT="${GUACAMELEE_PRESENT:-letterbox}"
  export GUACAMELEE_GL_DIAG="${GUACAMELEE_GL_DIAG:-0}"
  export GUACAMELEE_FBO_LIFECYCLE="${GUACAMELEE_FBO_LIFECYCLE:-0}"
  export GUACAMELEE_XPORT_DIAG="${GUACAMELEE_XPORT_DIAG:-0}"
  export GUACAMELEE_DEPTH_ONLY_READ_NONE="${GUACAMELEE_DEPTH_ONLY_READ_NONE:-0}"
  export GUACAMELEE_REAL_GLERROR="${GUACAMELEE_REAL_GLERROR:-0}"
  export GUACAMELEE_KHR_DEBUG="${GUACAMELEE_KHR_DEBUG:-0}"
  export GUACAMELEE_OP_RING="${GUACAMELEE_OP_RING:-0}"
  export GUACAMELEE_PIXEL_PROBE="${GUACAMELEE_PIXEL_PROBE:-0}"
  export GUACAMELEE_PIXEL_DUMP="${GUACAMELEE_PIXEL_DUMP:-0}"
  export GUACAMELEE_FBO_TRANSITION_DIAG="${GUACAMELEE_FBO_TRANSITION_DIAG:-0}"
  export GUACAMELEE_FBO_FORMAT_MATRIX="${GUACAMELEE_FBO_FORMAT_MATRIX:-0}"
  export GUACAMELEE_GL_ERROR_TRACE="${GUACAMELEE_GL_ERROR_TRACE:-0}"
  export GUACAMELEE_GL4ES_PACKED_DS_CAP="${GUACAMELEE_GL4ES_PACKED_DS_CAP:-0}"
  export GUACAMELEE_LIBGL_AUTOMIPMAP="${GUACAMELEE_LIBGL_AUTOMIPMAP:-0}"
  export GUACAMELEE_MIPMAP_DIAG="${GUACAMELEE_MIPMAP_DIAG:-0}"
  export GUACAMELEE_SKIP_BAD_MIPMAP="${GUACAMELEE_SKIP_BAD_MIPMAP:-0}"
  export GUACAMELEE_TITLE_DRAW_PROBE="${GUACAMELEE_TITLE_DRAW_PROBE:-0}"
  export GUACAMELEE_TITLE_STATE_DIAG="${GUACAMELEE_TITLE_STATE_DIAG:-0}"
  export GUACAMELEE_VAO_DIAG="${GUACAMELEE_VAO_DIAG:-0}"
  export GUACAMELEE_TITLE_OCCLUSION_DIAG="${GUACAMELEE_TITLE_OCCLUSION_DIAG:-0}"
  export LIBGL_AUTOMIPMAP="$GUACAMELEE_LIBGL_AUTOMIPMAP"
  export GUACAMELEE_ZERO_VIEWPORT="${GUACAMELEE_ZERO_VIEWPORT:-0}"
  export GUACAMELEE_HARDEXT_FIX="${GUACAMELEE_HARDEXT_FIX:-0}"
  export GUACAMELEE_RB_ZERO_SIZE="${GUACAMELEE_RB_ZERO_SIZE:-0}"
  export GUACAMELEE_FBO_TEXTURE_FALLBACK="${GUACAMELEE_FBO_TEXTURE_FALLBACK:-0}"
  export GUACAMELEE_UNIFY_DEPTH_STENCIL="${GUACAMELEE_UNIFY_DEPTH_STENCIL:-0}"
  export GUACAMELEE_RB_FORMAT_FIX="${GUACAMELEE_RB_FORMAT_FIX:-0}"
  export GUACAMELEE_FRONTEND_RB_SIZE_FIX="${GUACAMELEE_FRONTEND_RB_SIZE_FIX:-0}"
  export GUACAMELEE_FRONTEND_WIDTH="${GUACAMELEE_FRONTEND_WIDTH:-$GUACAMELEE_WIDTH}"
  export GUACAMELEE_FRONTEND_HEIGHT="${GUACAMELEE_FRONTEND_HEIGHT:-$GUACAMELEE_HEIGHT}"
  export TSPGL_DEPTH_ONLY_READ_NONE="$GUACAMELEE_DEPTH_ONLY_READ_NONE"
  export TSPGL_ZERO_VIEWPORT="$GUACAMELEE_ZERO_VIEWPORT"
  export TSPGL_RB_ZERO_SIZE="$GUACAMELEE_RB_ZERO_SIZE"
  export TSPGL_FBO_TEXTURE_FALLBACK="$GUACAMELEE_FBO_TEXTURE_FALLBACK"
  export TSPGL_UNIFY_DEPTH_STENCIL="$GUACAMELEE_UNIFY_DEPTH_STENCIL"
  export TSPGL_RB_FORMAT_FIX="$GUACAMELEE_RB_FORMAT_FIX"
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
  export XDG_RUNTIME_DIR=/tmp
  export TMPDIR=/tmp
  export SDL_VIDEODRIVER="${GUACAMELEE_SDL_VIDEODRIVER:-offscreen}"
  export LD_LIBRARY_PATH="$GLBRIDGE:$GAMEDIR/box86/native:$SYS/lib/arm-linux-gnueabihf:$SYS/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
  export BOX86_LD_LIBRARY_PATH="$GAMEDIR/box86/native:$GAMEDIR/box86/x86:$GAMEDIR/gamedata/lib32:$GAMEDIR/libs/x86"
  export BOX86_PREFER_WRAPPED=1
  export BOX86_X11THREADS=1
  export BOX86_DYNAREC="${GUACAMELEE_BOX86_DYNAREC:-1}"
  export BOX86_DYNAREC_BIGBLOCK="${GUACAMELEE_BOX86_DYNAREC_BIGBLOCK:-1}"
  export BOX86_LOG="${GUACAMELEE_BOX86_LOG:-0}"
  export BOX86_DLSYM_ERROR="${GUACAMELEE_BOX86_DLSYM_ERROR:-0}"
  export BOX86_DYNAREC_LOG="${GUACAMELEE_BOX86_DYNAREC_LOG:-0}"
  export SDL_VIDEO_GL_DRIVER="$GAMEDIR/gl4es/libGL.so.1"
  export GUACAMELEE_GL4ES_PATH="$GAMEDIR/gl4es/libGL.so.1"
  export SDL_VIDEO_EGL_DRIVER="$GLBRIDGE/libEGL.so.1"
  export LIBGL_GLES="$GLBRIDGE/libGLESv2.so.2"
  export LIBGL_EGL="$GLBRIDGE/libEGL.so.1"
  export LIBGL_ES="${GUACAMELEE_LIBGL_ES:-2}"
  export LIBGL_GL="${GUACAMELEE_LIBGL_GL:-21}"

  # gl4es' normal hardware discovery compiles desktop-GLSL probe shaders such
  # as `#version 120` + GL_IMG_uniform_buffer_object + layout(location=...).
  # That test is useful with a direct vendor GLES driver, but it is misleading
  # through the 32->64 proxy: the guest sees a GLES2 bridge while the presenter
  # owns a GLES3.2 Mali context. Keep gl4es on its conservative GLES2 baseline
  # during bring-up. Set GUACAMELEE_LIBGL_NOTEST=0 only for a controlled A/B.
  export LIBGL_NOTEST="${GUACAMELEE_LIBGL_NOTEST:-1}"

  export LIBGL_SHRINK="${GUACAMELEE_LIBGL_SHRINK:-4}"
  export LIBGL_FBOFORCETEX="${GUACAMELEE_LIBGL_FBOFORCETEX:-1}"
  export LIBGL_FB="${GUACAMELEE_LIBGL_FB:-1}"
  HARDEXT_FIX="${GUACAMELEE_HARDEXT_LIB:-$GAMEDIR/compat/libgua_hardext_fix.so}"
  if [ "$GUACAMELEE_HARDEXT_FIX" != "0" ] && [ ! -f "$HARDEXT_FIX" ]; then
    echo "Guacamelee hardext compatibility shim is missing"
    exit 2
  fi
  chmod 0755 "$GAMEDIR/box86/box86" "$GAMEDIR/gamedata/game-bin" "$LD"

  echo "bridge_presenter=$PRESENTER"
  echo "bridge_loader=$LD"
  echo "bridge_dimensions=${TSPGL_WIDTH}x${TSPGL_HEIGHT}"
  echo "box86_dynarec=$BOX86_DYNAREC bigblock=$BOX86_DYNAREC_BIGBLOCK log=$BOX86_LOG dlsym_error=$BOX86_DLSYM_ERROR dynarec_log=$BOX86_DYNAREC_LOG"
  echo "gl4es_es=$LIBGL_ES gl=$LIBGL_GL notest=$LIBGL_NOTEST shrink=$LIBGL_SHRINK fbotex=$LIBGL_FBOFORCETEX fb=$LIBGL_FB diag=$GUACAMELEE_GL_DIAG fbo_lifecycle=$GUACAMELEE_FBO_LIFECYCLE xport_diag=$GUACAMELEE_XPORT_DIAG depth_only_read_none=$GUACAMELEE_DEPTH_ONLY_READ_NONE real_glerror=$GUACAMELEE_REAL_GLERROR khr_debug=$GUACAMELEE_KHR_DEBUG op_ring=$GUACAMELEE_OP_RING pixel_probe=$GUACAMELEE_PIXEL_PROBE zero_viewport=$GUACAMELEE_ZERO_VIEWPORT rb_zero_size=$GUACAMELEE_RB_ZERO_SIZE fbo_texture_fallback=$GUACAMELEE_FBO_TEXTURE_FALLBACK unify_depth_stencil=$GUACAMELEE_UNIFY_DEPTH_STENCIL rb_format_fix=$GUACAMELEE_RB_FORMAT_FIX frontend_rb_size_fix=$GUACAMELEE_FRONTEND_RB_SIZE_FIX hardext_fix=$GUACAMELEE_HARDEXT_FIX"
  $GPTOKEYB2 "game-bin" -c "$GAMEDIR/guacamelee.ini" &
  pm_platform_helper "$GAMEDIR/box86/box86"
  if [ "$GUACAMELEE_HARDEXT_FIX" != "0" ]; then
    if [ "${GUACAMELEE_STRACE:-0}" != "0" ] && command -v strace >/dev/null 2>&1; then
      strace -ff -tt -T -s 256 \
        -o "$GAMEDIR/logs/abort-strace" \
        -e trace=clone,fork,vfork,execve,wait4,kill,tgkill,write,writev,read,openat,close,futex,clock_nanosleep \
        -e signal=all -E "LD_PRELOAD=$HARDEXT_FIX" \
        "$LD" --library-path "$LD_LIBRARY_PATH" "$GAMEDIR/box86/box86" "$GAMEDIR/gamedata/game-bin"
    else
      LD_PRELOAD="$HARDEXT_FIX" "$LD" --library-path "$LD_LIBRARY_PATH" "$GAMEDIR/box86/box86" "$GAMEDIR/gamedata/game-bin"
    fi
  else
    env -u LD_PRELOAD "$LD" --library-path "$LD_LIBRARY_PATH" "$GAMEDIR/box86/box86" "$GAMEDIR/gamedata/game-bin"
  fi
  result=$?
else
  echo "backend=legacy-direct-gles"
  export PORT_32BIT=Y
  export LD_LIBRARY_PATH="$GAMEDIR/box86/native:/usr/lib/arm-linux-gnueabihf:/usr/lib32:$GAMEDIR/libs/x86${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
  export BOX86_LD_LIBRARY_PATH="$GAMEDIR/box86/x86:$GAMEDIR/gamedata/lib32:$GAMEDIR/libs/x86"
  export SDL_VIDEO_GL_DRIVER="$GAMEDIR/gl4es/libGL.so.1"
  export LIBGL_SHRINK=4
  $GPTOKEYB2 "game-bin" -c "$GAMEDIR/guacamelee.ini" &
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
