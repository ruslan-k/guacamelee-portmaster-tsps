# Scratch VBO ring classifier — 2026-09-15

A 3-slot per-attribute scratch VBO ring was tested, rotating buffers for each
`OP_glUploadAttrib` while keeping the SDL logical-mode classifier active.

Result:

```text
swap 3 FBO1/game/window = nonblack=1952/6144
then: tspgl client fd=21 drop
```

The ring did not produce a working title path and did not provide evidence of a
fix. The run was classified as unsuccessful/early bridge termination, not as a
production regression.

The exact production presenter and bridge were restored from the remote backup
created before the test. Runtime verification:

```text
guacamelee_present 28668fbfc26bb4f207acefe436978923df6d545595ec7a16a0c58cdba1fb388
libEGL.so.1         f264443cf87daa740f0ad99de53292bcbcf07143e936cfc6d901efdc5f3e1be3
libGL.so.1          6b0c60b1f942a9a408d83bc6fdc46a44cbd7195aed482e7fe1cb43420f253253
game-bin            5aa2a2cc89d79912a4036ce5aca344fa6cc6f8767e346b8d271c3b76300aa063
```

No diagnostic preload remains. MainUI is active; game/presenter/gptokeyb2 are
not running. No production fix is claimed.
