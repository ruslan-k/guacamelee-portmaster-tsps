# Program-18 fragment classifiers — 2026-09-15

Three diagnostic shader-output classifiers were run with the proven SDL logical
mode correction active. Each rewrote only the matching textured program's
fragment output.

## Results

```text
classifier   swap 2             swap 3
solid        nonblack=4096/6144 nonblack=4096/6144
texture      nonblack=6144/6144 nonblack=6048/6144
colour       nonblack=0/6144    nonblack=3072/6144
```

The runs were intentionally bounded; the classifier changes alter shader
optimization and startup timing, so lack of a later swap-10 comparison is not
interpreted as a functional success/failure claim.

## Interpretation

- Solid output renders: raster/output/FBO path is capable of producing pixels.
- Texture-only output renders: texture sampling path works.
- Colour-only output renders partially: vertex color varying reaches the shader.
- The original multiplication path remains the failing interaction at the
  relevant title transition.

This does not justify shipping a shader rewrite. The remaining production-safe
investigation is the exact sampled vertex/color values and blend behavior at the
first blackening draw, compared with native output. All classifier binaries and
SDL shims were removed after the runs.

Production runtime was restored and verified; MainUI remains active and no game
processes remain.
