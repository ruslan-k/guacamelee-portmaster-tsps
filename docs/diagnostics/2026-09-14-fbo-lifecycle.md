# FBO lifecycle and compatibility A/B — 2026-09-14

## Baseline

The working baseline remains:

```text
1024x768
LIBGL_ES=2
LIBGL_GL=21
LIBGL_NOTEST=1
LIBGL_FBOFORCETEX=1
REAL_GLERROR=0
CPU-affinity patch enabled
```

The splash remains correctly centered. The post-splash frame remains a static black/fade frame.

## Stateful lifecycle result

The new bounded lifecycle trace shows the title renderer repeatedly uses application FBOs 2 and 4:

```text
GUA-FBO status fb=2 status=0x8cd7
GUA-FBO status fb=2 status=0x8cd6
GUA-FBO status fb=3 status=0x8cd7
GUA-FBO status fb=4 status=0x8cd7
GUA-FBO status fb=5 status=0x8cd7
GUA-FBO status fb=6 status=0x8cd7
```

The first application renderbuffer state is:

```text
fb=2 rb=2 fmt=0x8056 requested=0x0 actual=0x0 internal=0x8056
fb=2 rb=4 fmt=0x8d48 requested=0x0 actual=0x0 internal=0x8d48
fb=2 rb=3 fmt=0x81a5 requested=0x0 actual=0x0 internal=0x81a5
```

Attachments observed:

```text
fb=2 slot=0x8d00 type=0x8d41 name=3
fb=2 slot=0x8d20 type=0x8d41 name=4
```

`0x8d00` and `0x8d20` are depth/stencil attachment points. No color attachment was observed for these FBOs in the lifecycle window.

The title draws occur on the incomplete FBOs with a zero viewport:

```text
GUA-DRAW fb=2 viewport=0,0,0,0 op=DrawElements
GUA-DRAW fb=4 viewport=0,0,0,0 op=DrawArrays
```

The presenter default game FBO path draws separately with the expected viewport:

```text
GUA-DRAW fb=1 viewport=0,0,1024,768 op=DrawArrays
GUA-SWAP bound-fb=0 game-fbo=1 last-draw-fb=1
```

This is direct evidence that title rendering is routed through incomplete application FBOs before the final default-FBO pass.

## A/B results

### `LIBGL_FBOFORCETEX=0`

No material change:

- application FBO statuses remained incomplete;
- title draws still used FBO2/FBO4;
- black frame remained.

### `REAL_GLERROR=1`

The previous client-side fake `GL_NO_ERROR` was disabled. Mali errors became visible:

```text
0x500
0x502
0x506
```

The title frame did not recover. This confirms real backend errors, but does not yet identify the first causative operation.

### `ZERO_VIEWPORT=1`

Zero viewport calls were replaced with `1024x768` only for the opt-in run:

```text
GUA-FBO zero-viewport override 1024x768
GUA-DRAW fb=2 viewport=0,0,1024,768 op=DrawElements
```

The FBO statuses remained incomplete and the DRM frame remained black. Zero viewport is therefore a consequence/symptom of the incomplete application FBO path, not a standalone fix.

## Current root-cause boundary

Proven:

1. Presenter game FBO is complete.
2. Title rendering is performed on application FBO2/FBO4.
3. Those FBOs have no observed color attachment, zero-sized renderbuffer storage, and incomplete status.
4. Real Mali errors are generated during title rendering.
5. `FBOFORCETEX=0`, real `glGetError`, and zero-viewport overrides do not independently fix the frame.

The next actual fix must repair application FBO lifecycle/attachment semantics, not resolution, Box86 timing, input, or presenter letterboxing. Do not mask `glCheckFramebufferStatus` as complete.

## Source changes

Added opt-in diagnostic controls, all default-off:

- `GUACAMELEE_FBO_LIFECYCLE`
- `GUACAMELEE_LIBGL_FBOFORCETEX`
- `GUACAMELEE_REAL_GLERROR`
- `GUACAMELEE_ZERO_VIEWPORT`
- `GUACAMELEE_DEPTH_ONLY_READ_NONE`

`build_port.sh` now rebuilds the bridge before packaging, preventing source/binary diagnostic drift.
