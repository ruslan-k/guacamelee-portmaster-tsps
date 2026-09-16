# Guacamelee Box32 direct experiment

This is a separate, reversible experiment. It does not modify `portmaster/Guacamelee.sh` and does not use the ARMHF Box86/gl4es GLES socket bridge or `guacamelee_present`.

Pipeline:

```text
i386 Guacamelee -> AArch64 Box64 with BOX32 -> native wrapped SDL2/libGL -> AArch64 gl4es -> native EGL/GLES -> Mali
```

The launcher runs the original paid `gamedata/game-bin` from `gamedata/` beside `shaders.dat.ogl`, `resources.dat`, `misc.dat`, and `levels.dat`; no game executable or paid asset is included in this package. It shares the already extracted `gamedata/` but has its own log name. Remove the launcher and this directory to roll back.

The current TSPS image was checked before preparing this experiment: `/usr/trimui/lib/libEGL.so.1` and `/usr/trimui/lib/libGLESv2.so.2` are absent, while the matching libraries are present under `/usr/lib/`. The launcher prefers the requested `/usr/trimui/lib` paths and records a fallback to `/usr/lib` only when the preferred file is absent.

Required milestones in `logs/box32-direct.log`:

1. Box32 recognizes `game-bin` as i386.
2. Box64 selects wrapped/native SDL2 and libGL.
3. `BOX64_LIBGL` points to the bundled AArch64 gl4es.
4. gl4es initializes the native EGL/GLES backend.
5. game assets, shader compilation, draw, swap, and physical visible frame.

A Box64 `libGL.so.1 not found` message is not a valid result unless it occurs after the logged `BOX64_LIBGL` path was attempted.

Build provenance:

- Box64: v0.4.5, upstream commit `f9d58352e937c8448a921f978b81da8cd86419c7`, rebuilt with `BOX32=ON`, `BOX32_BINFMT=OFF` using the repository's Spruce-derived `Dockerfile.64` (Ubuntu 20.04 multiarch, glibc 2.31, pinned CMake 3.22.6). Primary tested binary has SHA-256 `ac602ca9aac22aaad86951b6fb0a68a62f08b4ee9ffd7ff2823a946267fc2d00` and includes both independently reviewable Box64 patches.
- gl4es: upstream commit `81547d986798e876de8b434193920b606a72363f`, AArch64 build from `~/Downloads/gl4es-aarch64-tsps.zip`, `NOX11=ON`, `GBM=OFF`, `DEFAULT_ES=2`, `USE_CLOCK=ON`, `GLX_STUBS=ON`, `EGL_WRAPPER=ON`. The archive's `libEGL-gl4es.so.1` remains an optional follow-up; the tested path uses system EGL/GLES.
- The direct experiment includes two separate i386 game-scoped compatibility shims. `libbox32_affinityshim.so` prevents the native affinity attr mutation that makes the following Box64 `pthread_create` return `EINVAL`; `libbox32_schedshim.so` overrides only the separately observed `pthread_setschedprio()` issue. Both are isolated behind `BOX64_LD_PRELOAD` and documented in `docs/diagnostics/box32-direct-affinity-wrapper.txt`.
- `src/x11stub.c` remains a historical diagnostic artifact only. The tested post-NULL-fix launcher does not require or package an X11 stub.
