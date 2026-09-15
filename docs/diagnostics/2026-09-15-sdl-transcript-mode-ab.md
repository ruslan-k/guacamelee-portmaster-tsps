# Native-vs-TSPS SDL transcript and offscreen mode A/B — 2026-09-15

## SDL transcript divergence

The same i386 SDL preload was run natively and on TSPS.

### Native Mesa

```text
SDL_GetDisplayBounds          rc=0 rect=0,0 3840x2160
SDL_GetDesktopDisplayMode     rc=0
SDL_CreateWindow               req=32x32 flags=0xA
SDL_GL_CreateContext           success
SDL_SetWindowFullscreen        flags=0x1023 rc=0
SDL_CreateWindow               req=3840x2160 flags=0x1023
SDL_GL_CreateContext           success
SDL_GetWindowSize              3840x2160
SDL_GL_GetDrawableSize         3840x2160
SDL_WINDOWEVENT events         present during initialization
```

The native object probe then showed `3840x2160` render-target metadata and a
nonzero final blit.

### TSPS baseline

The TSPS launcher uses `SDL_VIDEODRIVER=offscreen`. The same transcript showed:

```text
SDL_GetDesktopDisplayMode     rc=-1
SDL_GetDisplayBounds          rc=-1 rect=0,0 0x0
SDL_CreateWindow               req=0x0 flags=0x1023
SDL_GL_CreateContext           success
SDL_GetDrawableSize            1024x768
SDL_GetWindowSize              1024x768
SDL_GetCurrentDisplayMode      rc=0
SDL_WINDOWEVENT subtype=1     after context/size setup
```

This is the first concrete native-vs-TSPS initialization divergence. TSPS gets
correct size values later, but it misses the normal desktop display/mode path
and creates the offscreen window with `0x0` dimensions.

## Offscreen SDL compatibility A/B

A diagnostic i386 shim returned logical display mode/bounds `1024x768` and
changed zero-size `SDL_CreateWindow` requests to `1024x768`:

```text
GUA-SDL-FIX DesktopDisplayMode rc=0 size=1024x768
GUA-SDL-FIX Bounds rc=0 rect=0,0 1024x768
GUA-SDL-FIX CreateWindow req=1024x768 flags=0x3ff
GUA-SDL-FIX CurrentDisplayMode rc=0 size=1024x768
```

Result:

```text
swap 3 prompt nonblack=1952/6144
swap 10/30 black
FBO1/window remained black after the prompt transition
pure virtual method called: absent in this A/B
```

The A/B changed the early renderer viewport to a letterboxed
`0,96,1024,576`, proving the shim was active and changing initialization
state. It did not fix the later black title. The shim was removed.

## Interpretation

The missing offscreen display/mode contract is a real initialization
 divergence and should be preserved as an input to the eventual fix, but
normalizing it alone does not resolve the title. The remaining divergence is
later in the game's render-target/title-state initialization. Do not promote
the shim as a production fix without matching the native object lifetime and
render-target construction path.

Production runtime restored after the A/B:

```text
libGL.so.1 SHA-256:
6b0c60b1f942a9a408d83bc6fdc46a44cbd7195aed482e7fe1cb43420f253253
MainUI PID 4555 active
Guacamelee processes: none
```
