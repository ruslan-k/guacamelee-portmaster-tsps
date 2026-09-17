# Runtime manifest — Guacamelee Box32 direct

## Pinned tested runtime

```text
purpose                 diagnostic latest / not production replacement
upstream Box64 tested   6f55a18f5e3f5eaadd293b046e1428f4aca7bd50ce6462ceffc11a4fca4849d5
upstream gl4es local    ae5266a56dd4aa050bf0ddfe3127c2045e54753415095d9cc00dbf261149eca8
latest deployed libGL  37fe3fe0d86ace2bb1b746846396c48f788b6d8d3bfb5f53887accc6c78cf1a8
affinity shim           db0906899e080a1b63e8a1309e6bf631a45b6492ce56d90c761365b522feef33
sched shim              b34c54dc60ed2d0180cf1b77ecba386fd63e01c0bcfc9dea2ad57cff333955fd
```

Compatibility pieces retained in tested closure:

```text
Box64 low-pointer compatibility patch
Box64 NULL-display compatibility patch
game-local affinity shim
game-local scheduler shim
gl4es LIBGL_NOTEST maxcolorattach=1 / maxdrawbuffers=1 correction
hardext depthstencil=1 baseline needed for complete tested FBO
```

Diagnostic-only traces are not production-candidate patches. Direct-present
helper, pixel readback, FBO remap, FBO forcing, shader/color rewrites and
LIBGL_FB/FBOFORCETEX are excluded from clean series.

## Build

```text
container image: guacamelee-box32-toolchain:64
build command: cmake --build /build -j2
repro script: experiments/box32-direct/build-gl4es.sh
clean series: experiments/box32-direct/patches/clean-series.txt
```

The script records output SHA/file/GLIBC metadata. The current diagnostic source
is intentionally dirty because it contains forensic traces; it is not claimed as
a clean production build.

## Diagnostic builds

```text
P2 exact window log: /mnt/external/Hermes/guac-p2win-runtime.log
SHA-256=d09a404b5993999ef6e8c013a32609a355a927d607b48bc5b90e29e123751d48

P2 segment log: /mnt/external/Hermes/guac-p2full-runtime.log
SHA-256=7e0591478ae8ac3a04406dbdd8920c49fda359f5d1688a77772e60aabf3673de

P2 FPE chain log: /mnt/external/Hermes/guac-p2fpe2-runtime.log
SHA-256=53e44870fed18dac2394f7fed177f6b6548c1300d6c82b4bf138e2ad5bf89f2a
```
