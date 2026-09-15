# Porting status

## 2026-09-14: initial TSPS bridge checkpoint

### Confirmed locally

- The supplied GOG installer is a Makeself shell wrapper containing a second ZIP archive. Its game payload contains an i386 `game-bin`, x86 SDL2, x86 FMOD Ex 4.44.27, `resources.dat`, and the FSB media banks.
- The supplied game executable imports `libGL.so.1`, uses OpenGL 2.1-era fixed functions, and links against the x86 SDL2 and FMOD libraries from the installer.
- The supplied 32-bit PortMaster package contains ARMHF Box86 and gl4es, but no GLES bridge or AArch64 presenter.
- The bridge source is derived from the Galaxy on Fire 2 repository at commit `7471aaed0f3798a6556e7d16512f9e79a536b635`.
- The bridge client and presenter compile with Zig 0.13 for ARMHF and AArch64 respectively.
- `eglQuerySurface` and the bridge defaults now use `1024x768`, matching the GOG SDL drawable; the shared frame maximum height is 768.
- A controlled device A/B proved that the former 640x480 FBO cropped the 1024x768 game viewport and shifted the splash approximately 288 pixels to the right and upward. The production default is now 1024x768 letterboxed to 1280x720.

### Not yet confirmed

- A real TSPS menu launch with the GOG game data.
- Box86 startup on the target with the GOG SDL2 build.
- Presenter frames, input, audio, save writes, and return to PortMaster.
- Whether this GOG SDL2 build needs a target-specific SDL video backend or can use the inherited PortMaster backend.

### First device hypothesis

The initial 640x480 hypothesis is disproven for this SDL build. Use the 1024x768 guest surface and letterbox it to the 1280x720 panel. Keep 640x480 only as a controlled A/B override.
