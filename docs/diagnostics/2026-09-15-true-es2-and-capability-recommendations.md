# True GLES2 and capability recommendations — 2026-09-15

## Applied recommendation

The new recommendation was tested as an integration A/B:

- forced the bridge to request an actual GLES2 context instead of GLES3;
- forced gl4es capability state to advertise one color attachment, packed depth/stencil, and depth24;
- retained the SDL logical-mode correction used by the previous diagnostic run;
- ran the result on the real TSPS device.

The device still created a Mali GLES 3.2 context. The bridge reported:

```text
tspgl-srv: window 1280x720 GL context ok driver=KMSDRM swap_interval=0
GL vendor=ARM renderer=Mali-G57 version=OpenGL ES 3.2
```

The run reached a visible non-black state:

```text
swap 10/30: game nonblack=1952/6144
swap 10/30: window nonblack=1952/6144
```

This differs from the earlier classifier runs that returned to zero non-black pixels, but it does not yet prove a stable playable fix. The run also logged `SDL_GetNumVideoDisplays() failed` and was terminated after the diagnostic window.

## Classification

- The forced capability state did not cause a context failure.
- The tested combination produced a non-black title sample at the measured swaps.
- It was an evidence-only integration A/B, not promoted to production.
- Production was restored from the distribution archive after the test.

## Verified production state

```text
guacamelee_present  28668fbfc26bb4f207acefe436978923df6d545595ec7a16a0c58cdba1fb388
libEGL.so.1         f264443cf87daa740f0ad99de53292bcbcf07143e936cfc6d901efdc5f3e1be3
libGL.so.1          6b0c60b1f942a9a408d83bc6fdc46a44cbd7195aed482e7fe1cb43420f253253
game-bin            5aa2a2cc89d79912a4036ce5aca344fa6cc6f8767e346b8d271c3b76300aa063
```

MainUI is active and game/presenter/gptokeyb2 processes are absent.
