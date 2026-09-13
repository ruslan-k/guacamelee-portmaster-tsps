# Porting status

## 2026-09-14: initial TSPS bridge checkpoint

### Confirmed locally

- The supplied GOG installer is a Makeself shell wrapper containing a second ZIP archive. Its game payload contains an i386 `game-bin`, x86 SDL2, x86 FMOD Ex 4.44.27, `resources.dat`, and the FSB media banks.
- The supplied game executable imports `libGL.so.1`, uses OpenGL 2.1-era fixed functions, and links against the x86 SDL2 and FMOD libraries from the installer.
- The supplied 32-bit PortMaster package contains ARMHF Box86 and gl4es, but no GLES bridge or AArch64 presenter.
- The bridge source is derived from the Galaxy on Fire 2 repository at commit `7471aaed0f3798a6556e7d16512f9e79a536b635`.
- The bridge client and presenter compile with Zig 0.13 for ARMHF and AArch64 respectively.
- `eglQuerySurface` now consumes bounded `TSPGL_WIDTH` and `TSPGL_HEIGHT` values instead of fixed 640x480 literals.

### Not yet confirmed

- A real TSPS menu launch with the GOG game data.
- Box86 startup on the target with the GOG SDL2 build.
- Presenter frames, input, audio, save writes, and return to PortMaster.
- Whether this GOG SDL2 build needs a target-specific SDL video backend or can use the inherited PortMaster backend.

### First device hypothesis

Use a 640x480 guest surface and letterbox it to the 1280x720 panel. If the presenter reaches readiness but the game fails before its first frame, preserve that log and classify the failure at the SDL video, gl4es, bridge, or Box86 boundary before changing resolution or audio.
