# Isolated presenter context phase — 2026-09-15

## Applied

Implemented an isolated presentation candidate:

- created a second SDL GL context with `SDL_GL_SHARE_WITH_CURRENT_CONTEXT`;
- kept the game RPC on the original game context;
- switched to the shared presenter context for presentation;
- sampled the shared `game_color` texture with a GLES2 textured quad instead of using `glBlitFramebuffer`;
- switched back to the game context after swap;
- deleted the presenter context during cleanup.

The candidate compiled successfully and the device reported:

```text
tspgl-srv: shared presenter GLES context created
GL vendor=ARM renderer=Mali-G57 version=OpenGL ES 3.2
```

## Device result

The game context produced a non-black logical FBO at the early title boundary:

```text
swap 3 game FBO: nonblack=1952/6144
```

The existing window readback performed after switching back to the game context remained black. That readback is not sufficient to classify the physical presentation because the default framebuffer is context-local; the next required measurement is a presenter-context readback or synchronized KMS capture before/after `SDL_GL_SwapWindow`.

The candidate still logged the pre-existing game-side errors:

```text
GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT
GL_FRAMEBUFFER_UNSUPPORTED
glError (0x500)
```

No production fix is claimed.

## Restoration

Production was restored and verified:

```text
guacamelee_present 28668fbfc26bb4f207acefe436978923df6d545595ec7a16a0c58cdba1fb388
libEGL.so.1        f264443cf87daa740f0ad99de53292bcbcf07143e936cfc6d901efdc5f3e1be3
libGL.so.1         6b0c60b1f942a9a408d83bc6fdc46a44cbd7195aed482e7fe1cb43420f253253
game-bin           5aa2a2cc89d79912a4036ce5aca344fa6cc6f8767e346b8d271c3b76300aa063
```

MainUI is active; target processes are absent.
