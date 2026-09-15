# Post-clear frame graph and state-preserving diagnostics

Date: 2026-09-15
Target: TrimUI Smart Pro / Spruce

## Diagnostic state fixes

Before this run, two observer-induced state leaks were corrected:

- `fbo_census()` now saves/restores `GL_READ_FRAMEBUFFER_BINDING` and `GL_DRAW_FRAMEBUFFER_BINDING` independently, along with read buffer and pack alignment.
- `present_swap()` now saves the real pre-swap READ/DRAW framebuffer bindings and restores them independently after `SDL_GL_SwapWindow()` instead of always binding `game_fbo`.

The presenter still used the normal production routing (`PRESENT_LAST_FBO=0`); no presentation workaround was active.

## Post-clear graph

The transition trace was armed after swap 100 and retained a bounded post-clear window. It captured this repeated sequence on FBO2:

```text
FBO2 nonblack=2583/6144
op=glClear mask=0x4500 clear=0,0,0,1
FBO2 nonblack=0
op=glDrawElements program=49 count=483 type=0x1403 index=24
FBO2 nonblack=2583/6144
```

One post-clear event also triggered a one-shot full readback:

```text
GUA-COLOR-SIG seq=18107 ms=20914748 fb=2
op=glDrawElements
nonblack=2583
program=49
args=4,483,5123,24,0,0
hash=f689733d8e6dfed6

GUA-TITLE-FULL stage=post-clear seq=18107 fb=2
nonblack=60883/786432
bbox=196,266..807,487
```

The clear itself was:

```text
program=21
mask=1111
scissor=0
box=0,0,1280,720
clear=0,0,0,1
viewport=0,0,1024,768
```

The transition was therefore not a permanent clear-to-black failure. Program 49 draws restore a non-black FBO2 image after every clear in the observed post-clear window.

## Render graph consequence

The game continues to bind logical framebuffer 0 before swap. The presenter maps that to `game_fbo=1`, while the useful post-clear content is in FBO2. No game-side `glBlitFramebuffer`, `glCopyTexImage2D`, or `glCopyTexSubImage2D` compose operation was observed in the relevant traffic.

This leaves two separate facts:

1. FBO2 does receive non-black content after the clear.
2. The logical default/game FBO1 remains the presenter’s normal swap source and does not receive an explicit game-side compose.

`PRESENT_LAST_FBO=1` can expose an intermediate FBO2 frame physically, but remains diagnostic-only because FBO choice changes over time and is not OpenGL swap semantics.

## Scene state

The game log still reports:

```text
Loaded binary scene graph 'Map_Intro_StartScreen.level.bin'
Unable to spawn P0. No spawn point
Found 0 SDL_Joysticks
Found 0 SDL_GameControllers
```

The presenter independently detects an Xbox 360 controller and publishes pad state. The current client does not consume that shared pad state as SDL joystick input; keyboard ingress is provided separately by gptokeyb2.

## Interpretation

The post-clear graph rules out the hypothesis that program 49 never renders anything. It also rules out a permanently black FBO caused by the clear alone. The next causal boundary is now one of:

- why the non-black FBO2 result is not the logical framebuffer 0 output;
- whether the game is intentionally rendering a transient splash/fade while waiting for player activation;
- whether the title texture is sampled by the post-clear program49 path;
- whether Box86 timing changes the scene transition.

No shader or fade skip was applied.

## Restoration

After the run:

```text
libGL.so.1 SHA-256:
6b0c60b1f942a9a408d83bc6fdc46a44cbd7195aed482e7fe1cb43420f253253
Guacamelee/game helper processes: none
```

Raw late-run log: `2026-09-15-postclear-graph-late.log.gz`.
