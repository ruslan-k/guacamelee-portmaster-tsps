# VAO/EBO shadow phase — 2026-09-15

## Applied

Implemented and built the next production-code phase in `src/glbridge/server_gl.c`:

- added shadow VAO records with per-VAO element-buffer ownership;
- tracked the source array buffer with attribute metadata;
- made `glBindVertexArray` select the shadow record;
- made `glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ...)` update the selected VAO;
- made draw-element routing consult the selected VAO EBO instead of treating any global/VAO state as indexed input.

The ARM/AArch64 bridge build succeeded and `git diff --check` passed.

## Device test

The real-device A/B did not reach Guacamelee. The PortMaster launcher stopped before startup because this current device session lacks `/dev/shm/portmaster`:

```text
mkdir: can't create directory '/dev/shm/portmaster': No such file or directory
```

Therefore this is a build-verified code change, not a valid graphics A/B result. No visual conclusion is made.

## Restoration

Production was restored and read back:

```text
guacamelee_present 28668fbfc26bb4f207acefe436978923df6d545595ec7a16a0c58cdba1fb388
libEGL.so.1        f264443cf87daa740f0ad99de53292bcbcf07143e936cfc6d901efdc5f3e1be3
libGL.so.1         6b0c60b1f942a9a408d83bc6fdc46a44cbd7195aed482e7fe1cb43420f253253
game-bin           5aa2a2cc89d79912a4036ce5aca344fa6cc6f8767e346b8d271c3b76300aa063
```

MainUI is active and game/presenter/helper processes are absent.
