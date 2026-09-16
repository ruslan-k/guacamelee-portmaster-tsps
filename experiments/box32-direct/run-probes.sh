#!/bin/sh
set -eu
# Controlled TSPS runner. It never kills MainUI; caller must use menu ownership
# or explicitly stop only the probe process before invoking this script.
PROBE=${1:?probe path on TSPS}; LOG=${2:?log path on TSPS}; BOX64=${BOX64:?Box64/Box32 path}; GL4ES=${GL4ES:?AArch64 gl4es path}; NATIVE_EGL=${NATIVE_EGL:-/usr/trimui/lib/libEGL.so.1}; NATIVE_GLES=${NATIVE_GLES:-/usr/trimui/lib/libGLESv2.so.2}
export SDL_VIDEODRIVER=KMSDRM SDL_KMSDRM_REQUIRE_DRM_MASTER=0
export SDL_VIDEO_GL_DRIVER="$GL4ES" SDL_VIDEO_EGL_DRIVER="$NATIVE_EGL"
export BOX64_LIBGL="$GL4ES" BOX64_PREFER_WRAPPED=1 BOX64_WRAP_EGL=1
export BOX64_LOG=2 BOX64_DLSYM_ERROR=1 BOX64_SHOWSEGV=1 BOX64_SHOWBT=1
export LIBGL_GLES="$NATIVE_GLES" LIBGL_EGL="$NATIVE_EGL" LIBGL_ES=2 LIBGL_GL=21 LIBGL_NOTEST=1
export LIBGL_LOGSHADERERROR=1 LIBGL_SILENTSTUB=0
unset LIBGL_FB LIBGL_SHRINK BOX64_LD_PRELOAD
printf 'PROBE start=%s\n' "$PROBE" >"$LOG"
set +e
"$BOX64" "$PROBE" >>"$LOG" 2>&1
result=$?
set -e
printf 'PROBE exit_code=%s\n' "$result" >>"$LOG"
exit "$result"
