# Per-draw FBO1 classifier: program 28/18 — 2026-09-15

## Diagnostic change

The bridge was rebuilt with `GUACAMELEE_TITLE_DRAW_ALL=1`. In addition to the
existing FBO2 title probe, it read a 16x16 pixel sample immediately after each
FBO1 draw whose current program was 28 or 18. This was a diagnostic build only.

## Observed sequence

At the prompt-to-black boundary:

```text
seq=7802  FBO1 program=18  texture unit0=3  nonblack=768/768
seq=7811  FBO1 program=18  texture unit0=3  nonblack=0/768
```

The same pattern appears earlier:

```text
seq=5254  FBO1 program=18  nonblack=768/768
seq=5263  FBO1 program=18  nonblack=0/768
```

At the next frame:

```text
seq=7896  FBO1 program=28  nonblack=0/768
seq=7919  FBO1 program=18  nonblack=0/768
seq=7928  FBO1 program=18  nonblack=0/768
```

The swap-level result matches the per-draw classifier:

```text
swap 8: prompt path already loses content after the second program-18 draw
swap 9: program 28/18 path remains black
swap 10: FBO1/window nonblack=0
```

## Interpretation

The second FBO1 `program=18` draw is the first observed operation that changes
the sampled FBO1 region from fully nonblack to fully black. Program 28 is
already too late in the following frame to be the initial cause.

Both program-18 samples report sampler unit 0 bound to texture 3, so the next
narrow investigation is the native-vs-TSPS state/content of texture 3 and the
full uniform/vertex state at the two consecutive program-18 draws. The current
probe does not yet prove whether the black result comes from texture content,
vertex coverage, or shader output; no workaround is promoted.

## Restoration

The diagnostic presenter/bridge was removed after the run and the exact
production backups restored. MainUI remained active and no game processes were
left running.
