# Clean compose/source correlation — 2026-09-15

## Profile

One clean 45-second run used the production graphics path with bounded
observation only:

```text
LIBGL_NOTEST=1
LIBGL_ES=2
LIBGL_GL=21
LIBGL_FB=1
BOX86_DYNAREC=1
BOX86_LOG=0
GUACAMELEE_GL_DIAG=1
GUACAMELEE_FBO_LIFECYCLE=1
GUACAMELEE_PIXEL_PROBE=1
GUACAMELEE_OP_RING=1
```

A guest-side i386 probe captured the actual `glBlitFramebuffer` calls. Probe
SHA-256:

```text
a322126162a39e00178e1aa1eaa0aae7197107f645506bcbaaacb6dfff81fa0c
```

## Guest state at blit calls

The same state was observed for the first 32 calls:

```text
EDI=0x8c2ed20
read=0x8c2ed60
read width/height=0/0
draw=0x8c2edb4
draw width/height=0/0
meta=NULL
parent width/height=0/0
globals 0x8c36704/0x8c36708=0/0
src=0,0..0,0 dst=0,0..0,0
```

## Pixel and draw correlation

The same run reached a non-black controller prompt at swap 3:

```text
FBO1/app nonblack=2080/6144
window nonblack=2016/6144
hash=675ae5fc56b53803
```

By swap 10, the normal logical default/game FBO path was black:

```text
FBO1/game/window nonblack=0/6144
hash=46d2505958464383
```

At swap 300, the sampled application FBO2 was also black:

```text
FBO2 nonblack=0/6144
full readback nonblack=0/786432
bbox=-1,-1..-1,-1
FBO1 nonblack=0/6144
window nonblack=0/6144
```

The last application draw target at that checkpoint was FBO2, while the normal
presenter source remained FBO1:

```text
last-draw-fb=2 source-fb=1 game-fbo=1
```

No `pure virtual method called` message occurred in this clean baseline run.
That message occurred only in the late metadata-mutation A/B, so it is
classified as probe-induced/lifecycle-corruption evidence, not a baseline
root cause.

## Interpretation

This clean correlation does not support the zero-area blits as the immediate
cause of the final black title frame: the late source FBO2 sample is already
black while the same zero-area calls continue. The earlier post-clear run also
proved a separate interval where FBO2 becomes non-black after program 49.
Therefore the timeline contains more than one render state, and the exact
zero-area call must not be promoted to a global fix.

Current active boundary remains the game-side title/fade/render-state path and
its transition into/out of FBO2. The application display globals are still
incorrect and must be fixed upstream if the render-target path is repaired,
but the late metadata A/B showed that changing them alone does not create
pixels.

## Restoration

After the run:

```text
production libGL.so.1 SHA-256:
6b0c60b1f942a9a408d83bc6fdc46a44cbd7195aed482e7fe1cb43420f253253
Guacamelee/game helper processes: none
MainUI PID 4154 active
Temporary probe removed
```

Raw log:

```text
/tmp/guacamelee-clean-compose.log
```
