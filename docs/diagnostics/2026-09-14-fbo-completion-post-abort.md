# Application FBO completion chain and post-FBO abort — 2026-09-14

## Baseline preserved

The physical runs kept the original PortMaster gl4es binary unchanged:

```text
libGL.so.1 SHA-256: 6b0c60b1f942a9a408d83bc6fdc46a44cbd7195aed482e7fe1cb43420f253253
1024x768
LIBGL_ES=2
LIBGL_GL=21
LIBGL_NOTEST=1
LIBGL_FB=1
BOX86_DYNAREC=1
CPU affinity patch enabled
```

## Proven FBO causes

The original gl4es conservative path left `maxcolorattach=0`. The ARMHF interposer fixes only that field and proves it was loaded:

```text
GUA-HARDEXT notest=1 es=2 fbo=1 maxcolorattach=0->1 maxdrawbuffers=1
```

The bridge then receives `GL_COLOR_ATTACHMENT0` instead of rejecting it.

The application sent zero-size renderbuffer storage:

```text
RGBA4              0x0
0x8d48             0x0
DEPTH_COMPONENT16  0x0
```

The opt-in `GUACAMELEE_RB_ZERO_SIZE=1` override changes those dimensions to `1024x768`.

The `0x8d48` input value is not a depth-stencil enum; it is `GL_RENDERBUFFER_ALPHA_SIZE`. The opt-in `GUACAMELEE_RB_FORMAT_FIX=1` remaps it to `GL_DEPTH24_STENCIL8` (`0x88f0`). The depth/stencil attachment then becomes one compatible renderbuffer:

```text
GUA-FBO format override 0x8d48 -> 0x88f0
GUA-FBO depth-stencil rb=4
GUA-FBO unify depth attachment rb=4
fb=2 depth rb=4
fb=2 stencil rb=4
```

The application color texture also lacked storage on the observed path. `GUACAMELEE_FBO_TEXTURE_FALLBACK=1` allocates the missing color texture at `1024x768`:

```text
GUA-FBO texture fallback tex=2 -> 1024x768
```

## Physical result with all targeted FBO fixes

With all four opt-in switches enabled:

```text
GUACAMELEE_HARDEXT_FIX=1
GUACAMELEE_RB_ZERO_SIZE=1
GUACAMELEE_FBO_TEXTURE_FALLBACK=1
GUACAMELEE_UNIFY_DEPTH_STENCIL=1
GUACAMELEE_RB_FORMAT_FIX=1
GUACAMELEE_LIBGL_FBOFORCETEX=0
```

All observed application FBO checks are complete:

```text
FBO2 first status  = 0x8cd5
FBO2 final status  = 0x8cd5
FBO3 status        = 0x8cd5
```

There are no `GL_FRAMEBUFFER_INCOMPLETE` messages and no `GUA-DRAW` marker yet. The screen remains black and the game exits with:

```text
exit_code=134 (SIGABRT)
```

The game reaches:

```text
Opened archive misc.dat
Opened archive levels.dat
Opened archive resources.dat
Completed parsing file: preinit.ini
Loading shader cache...success!
```

It aborts before `Map_Intro_StartScreen.level.bin`, before the first application draw, and before title-scene rendering. `BOX86_LOG=1` changes timing but does not move this boundary. The logs contain no explicit game error or backtrace.

The wrong-ELF-class warning from child processes remains:

```text
ld.so: object .../libgua_hardext_fix.so ... wrong ELF class: ELFCLASS32
```

The ARMHF shim itself is loaded successfully by the game, but its `LD_PRELOAD` is inherited by an AArch64 child. This should be cleaned up separately; it is not the FBO completeness failure.

## Interpretation

The FBO blocker is now isolated and the four targeted compatibility changes make all observed application FBOs complete. The remaining blocker is a post-video-initialization/game-state abort before title-scene activation. Do not add more FBO status or viewport masking. The next diagnostic should capture the abort boundary with Box86-native signal/backtrace support or a short attach-only trace around the transition after shader/resource initialization.

All switches remain default-off in the launcher. The production gl4es library was never replaced.
