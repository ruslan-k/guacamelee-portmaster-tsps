# Full-FBO GOOD/BAD classifier — 2026-09-15

## Correction to the 16x16 result

The earlier 16x16 classifier was local. A full 1024x768 readback showed that
some apparently black samples were only black at the sampled corner while the
full FBO still contained content.

Example:

```text
seq=5236  program=18  full nonblack=156010/786432
seq=5245  program=18  full nonblack=17976/786432
```

The full framebuffer was not black after the second draw in this early pair.

## Actual full-FBO transitions

Later adjacent program-18 draws produced genuine full-FBO transitions:

```text
seq=7120  program=18  full nonblack=28492/786432
seq=7527  program=18  full nonblack=0/786432

seq=7651  program=18  full nonblack=161766/786432
seq=7660  program=18  full nonblack=0/786432
```

The zero result includes:

```text
min=0 max=0 bbox=-1,-1..-1,-1
```

Thus the second program-18 draw remains the first confirmed full-FBO black
writer in the later repeated pattern, but the exact source of its black output
is still unresolved.

## Current discriminator

Both good and bad draws use program 18 and texture unit 0 -> texture 3. The
next required evidence is texture 3's state/content immediately before each
draw, plus the dynamic program-18 state. The current full readback proves the
destination effect, not whether the source texture changed before the bad draw.

No production workaround was selected. The diagnostic bridge was removed and
the exact production presenter/bridge backups were restored.
