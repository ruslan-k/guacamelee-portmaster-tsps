# Native x86/Mesa reference — 2026-09-15

## Environment

The same i386 GOG `game-bin` and required game data were run natively on the
host under Mesa Radeon OpenGL, using the i386 system loader and the game's
x86 SDL2/FM0D libraries. This is a reference run only; it did not touch TSPS.

Native renderer:

```text
OpenGL vendor:   AMD
OpenGL renderer: AMD Radeon Graphics (radeonsi)
OpenGL version:  4.6 Compatibility Profile Mesa 26.2.1
SDL drawable:    3840x2160
SDL window:      3840x2160
```

The run opened all three archives and parsed `preinit.ini`, `gameSettings.ini`,
and `postinit.ini`. It was stopped after the bounded 20-second capture.

## Native guest blit result

The same i386 guest probe was loaded with `LD_PRELOAD`. It resolved both
functions:

```text
GUA-GUEST-PROBE resolve name=glBlitFramebuffer
GUA-GUEST-PROBE resolve name=glBlitFramebufferEXT
```

The native game-side object state was:

```text
read width/height  = 3840/2160
draw width/height  = 3840/2160
src                = 0,0..3840,2160
dst                = 0,2160..3840,0
mask               = 0x4000
filter             = 0x2601
```

This is the decisive comparison with TSPS:

```text
TSPS:   read/draw=0/0, src/dst=0,0..0,0
Native: read/draw=3840/2160, nonzero source/destination rectangles
```

The native process also reported zero SDL controllers, so that condition is
not by itself a TSPS-specific failure.

## Conclusion

The zero-area blits are not intentional no-op traffic in the working native
build. The same game and same guest call path use nonzero render-target sizes
and a real FBO-to-default-FBO blit on native Mesa.

This proves the TSPS guest-side render-target metadata initialization/dataflow
is wrong. The native run also shows the application object dimensions are
populated independently of the two TSPS globals that remain zero; therefore a
production fix should reproduce the native object initialization path, not
simply write 1024/768 into the globals or patch the final blit.

Raw native log:

```text
/tmp/guac-native-probe.log
```
