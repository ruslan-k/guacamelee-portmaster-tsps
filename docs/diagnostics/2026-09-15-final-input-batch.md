# Final remaining-input batch — 2026-09-15

## EBO/index capture

Late title draws use an element buffer rather than client-index uploads:

```text
bound_element = 90
type          = GL_UNSIGNED_SHORT
index offsets = 0xaa4, 0xa74, 0x570
counts        = 24, 24, 642, ...
```

The close GOOD/BAD transition pair itself uses `DrawArrays`, so no client index
payload exists for that pair. The separate EBO path is now identified; its
referenced content is stable game geometry data, not an untracked client blob.

## Final deterministic-input status

The following were measured for the relevant FBO1/program18 path:

```text
SDL/offscreen mode and RT metadata
FBO / viewport / read-draw buffers
blend / depth / stencil / cull
program and shader source
texture 3 sampled source
full guest attribute payload hashes
array metadata and buffer IDs
EBO path and indexed offsets
one-shot glFinish classifier
```

No production-side mutation changed the black result. The remaining failure is
not explained by the measured guest payloads or ordinary bridge state.

## Production restoration

The exact production binaries were restored from the distribution package and
verified on TSPS:

```text
guacamelee_present:
28668fbfc26bb4f207acefe436978923df6d545595ec7a16a0c58cdba1fb3888

libEGL.so.1:
f264443cf87daa740f0ad99de53292bcbcf07143e936cfc6d901efdc5f3e1be3

game-bin:
5aa2a2cc89d79912a4036ce5aca344fa6cc6f8767e346b8d271c3b76300aa063
```

No diagnostic preload remains. MainUI is active and game/presenter/gptokeyb2
are not running.
