# Guacamelee Box32 direct experiment

This is a separate, reversible experiment. It does not modify `portmaster/Guacamelee.sh` and does not use the ARMHF Box86/gl4es GLES socket bridge or `guacamelee_present`.

Pipeline:

```text
i386 Guacamelee -> AArch64 Box64 with BOX32 -> native wrapped SDL2/libGL -> AArch64 gl4es -> native EGL/GLES -> Mali
```

The launcher is intended to be installed as a second temporary PortMaster menu entry. It shares the already extracted `gamedata/` but has its own log name. Remove the launcher and this directory to roll back.

The current TSPS image was checked before preparing this experiment: `/usr/trimui/lib/libEGL.so.1` and `/usr/trimui/lib/libGLESv2.so.2` are absent, while the matching libraries are present under `/usr/lib/`. The launcher prefers the requested `/usr/trimui/lib` paths and records a fallback to `/usr/lib` only when the preferred file is absent.

Required milestones in `logs/box32-direct.log`:

1. Box32 recognizes `game-bin` as i386.
2. Box64 selects wrapped/native SDL2 and libGL.
3. `BOX64_LIBGL` points to the bundled AArch64 gl4es.
4. gl4es initializes the native EGL/GLES backend.
5. game assets, shader compilation, draw, swap, and physical visible frame.

A Box64 `libGL.so.1 not found` message is not a valid result unless it occurs after the logged `BOX64_LIBGL` path was attempted.

Build provenance:

- Box64: v0.4.5, upstream commit `f9d58352e937c8448a921f978b81da8cd86419c7`, rebuilt in Ubuntu 20.04/glibc 2.31 with `BOX32=ON`, `BOX32_BINFMT=OFF`. The direct build adds low-memory copies for `glGetString`/`glGetStringi` and GLX strings because native AArch64 pointers crossed 4 GiB in Box32.
- gl4es: upstream commit `81547d986798e876de8b434193920b606a72363f`, AArch64 build from `~/Downloads/gl4es-aarch64-tsps.zip`, `NOX11=ON`, `GBM=OFF`, `DEFAULT_ES=2`, `USE_CLOCK=ON`, `GLX_STUBS=ON`, `EGL_WRAPPER=ON`. The archive's `libEGL-gl4es.so.1` remains an optional follow-up; the first test uses system EGL/GLES.
- The direct experiment includes an i386 `libbox32_schedshim.so` that overrides only `pthread_setschedprio()` to return success. It is isolated behind `BOX64_LD_PRELOAD`; the native Box32 wrapper otherwise deadlocked in that guest call before SDL/KMSDRM startup.
- The package also includes an AArch64 `libX11.so.6` dlopen stub. Direct KMSDRM does not use X11, but the gl4es/GLX probe otherwise entered Box64's missing-X11 error path. The stub is not a desktop X11 implementation.
