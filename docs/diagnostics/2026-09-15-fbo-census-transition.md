# FBO census and nonblack-to-black transition

Date: 2026-09-15
Target: TrimUI Smart Pro / Spruce

## Clean profile

The run used the coherent pinned gl4es baseline with server-side FBO remaps disabled:

```text
LIBGL_NOTEST=1
LIBGL_ES=2
LIBGL_GL=21
LIBGL_FB=1
LIBGL_FBOFORCETEX=1
GL4ES RB/texture/viewport coherence fixes enabled
server RB/FBO workarounds disabled
PRESENT_LAST_FBO=0
PRESENT_SET_READ_BUFFER=0
HOLD_SWAP=0
GL_ERROR_TRACE=0
```

No white texture/color classifier, magenta classifier, hold, or last-FBO routing override was active.

## Live FBO census

The presenter tracked FBO1 through FBO6. At every checkpoint all six were complete and had a texture color attachment:

```text
status=0x8cd5
color-type=0x1702
color attachment=GL_TEXTURE
DRAW_BUFFER0=0x8ce0
READ_BUFFER=0x8ce0
```

Five-region probes showed:

```text
swap 10:  FBO1 nonblack=2080/6144; FBO2..FBO6=0
swap 30:  FBO1 nonblack=2080/6144; FBO2..FBO6=0
swap 50:  all sampled FBOs=0
swap 100: FBO2 nonblack=2925/6144; FBO1/FBO3..FBO6=0
swap 150: FBO2 nonblack=2583/6144; others=0
swap 200: FBO2 nonblack=2583/6144; others=0
swap 250: FBO2 nonblack=2583/6144; others=0
swap 300: FBO2 nonblack=2583/6144; others=0
swap 500+: all sampled FBOs=0
```

This excludes FBO4 as a hidden non-black final render target. FBO2 is the only offscreen target that carries the useful intermediate frame.

## Exact transition

A bounded transition trace was run after swap 100. It captured:

```text
GUA-COLOR-SIG seq=11330 ms=18805848 fb=2 nonblack=327
GUA-COLOR-TRANS seq=11456 ms=18805890 fb=2
op=glClear
before=6144
after=0
hash_before=f689733d8e6dfed6
hash_after=46d2505958464383
program=21
blend=1
eq=0x8006
src=0x302
dst=0x303
mask=1111
viewport=0,0,1024,768
args=17664,0,0,0,0,0
```

The corresponding swap timeline is:

```text
swap=100 ms=19054774 last-draw-fb=2 source-fb=1
transition glClear seq=18073 ms=19054795
swap=101 ms=19054799 last-draw-fb=2 source-fb=1
```

The `glClear` mask `0x4500` includes color/depth/stencil. It is a legal frame-start clear, not an invalid-FBO event. After the clear, the title pass issues repeated `DrawElements` calls with program 49, but the sampled color remains black.

No `glBlitFramebuffer`, `glCopyTexImage2D`, or other explicit compose operation was observed in the game traffic. The game binds logical framebuffer 0 (mapped by the presenter to game FBO1) before swap, but does not copy FBO2 into it.

## Scene state

The internal game log confirms:

```text
Loaded binary scene graph 'Map_Intro_StartScreen.level.bin'
Level load time: 2.006820 secs
Unable to spawn P0. No spawn point
```

The game also reports `Found 0 SDL_Joysticks` and `Found 0 SDL_GameControllers`; the presenter itself sees an Xbox 360 controller and publishes button/axis state through the shared frame header. The PortMaster `gptokeyb2` mapping is keyboard-based (`start=enter`, `a=z`), so this input discrepancy requires separate verification but is now relevant because FBO/fragment/presentation gates are cleared.

## Interpretation

The first nonblack-to-black event is a normal game-issued `glClear` at the start of the title/fade pass. The later black result is not caused by a hidden FBO, incomplete attachment, draw buffer, scissor, or presenter blit failure. The remaining candidates are:

1. game title/fade state remaining black after the clear;
2. missing input/player activation (`P0` has no spawn point);
3. missing logical compose path from the game’s offscreen render target to framebuffer 0;
4. a program-49/uniform state that intentionally produces black during this state.

## Restoration

After the run the production library was restored and checked:

```text
libGL.so.1 SHA-256:
6b0c60b1f942a9a408d83bc6fdc46a44cbd7195aed482e7fe1cb43420f253253
Guacamelee/game helper processes: none
```

Raw log: `2026-09-15-fbo-census-transition.log.gz`.
