# SDL mode compatibility A/B with guest object state — 2026-09-15

A combined i386 preload applied only the narrow SDL offscreen compatibility
change and captured guest object state in the same run.

## Early SDL corrections

```text
SDL_GetDesktopDisplayMode -> 1024x768, rc=0
SDL_GetDisplayBounds      -> 0,0 1024x768, rc=0
SDL_CreateWindow          -> request changed from 0x0 to 1024x768
SDL_GetCurrentDisplayMode -> 1024x768, rc=0
```

## Resulting guest object state

At the first real blit call, without any late metadata mutation:

```text
read width/height  = 1024/768
draw width/height  = 1024/768
src                = 0,0..1024,768
dst                = 0,768..1024,0
```

The global variables remained zero, so this proves the native-equivalent
render-target object is populated through the SDL/window initialization path,
not through a blind global memory write.

The corrected path also produced the expected letterboxed title viewport:

```text
viewport=0,96,1024,576
```

## Visual result

The A/B produced the controller prompt frame:

```text
swap 3 nonblack=1952/6144
window nonblack=1952/6144
```

The later title state remained black:

```text
swap 10/30 nonblack=0/6144
```

Therefore the offscreen SDL mode/display contract is the first native-vs-TSPS
initialization divergence and it is the missing source of the nonzero RT
metadata/blit geometry. Fixing that divergence alone does not fix the later
title/fade state.

The shim was removed after the run; no production change was made.
