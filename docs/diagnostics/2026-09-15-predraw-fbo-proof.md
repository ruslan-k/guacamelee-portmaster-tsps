# PRE-draw FBO proof — 2026-09-15

A diagnostic bridge captured full FBO1 readback immediately before and after
late program-18 draws.

Observed:

```text
seq=8069 PRE   nonblack=0/786432
seq=8069 POST  nonblack=0/786432

seq=8181 PRE   nonblack=0/786432
seq=8181 POST  nonblack=0/786432
```

The source texture remained nonblack:

```text
texture 3 nonblack=462/768
hash=204cbad35a34b548
```

Therefore these late program-18 draws are not the black writer. They receive an
already-black destination. The previous conclusion that the second program-18
draw was necessarily causal is corrected: post-draw-only readback was
insufficient.

The root boundary must be found earlier, between the last full nonblack FBO1
readback and the first full-black PRE readback. This is now the correct next
trace target. No production workaround was promoted.

Production was restored from distribution binaries and verified; MainUI remains
active and game/presenter/gptokeyb2 are stopped.
