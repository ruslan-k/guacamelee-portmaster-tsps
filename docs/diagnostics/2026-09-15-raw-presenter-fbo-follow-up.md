# Raw presenter framebuffer follow-up — 2026-09-15

## Recommendation applied

The previous isolated presenter path used `G.glBindFramebuffer`, but that entry point is wrapped for game-context framebuffer-0 emulation and replaces `0` with `game_fbo`. That wrapper must not run while `present_ctx` is current.

The presenter path now uses the raw `real_bind_fb` and `real_check_fb` entry points. The candidate also keeps:

- a shared second SDL context;
- shader-only presenter program;
- presenter-local VAO/VBO;
- shared `game_color` texture;
- `glFinish()` before switching contexts;
- presenter-context readback.

## Device evidence

The real TSPS run now reports:

```text
shared presenter GLES context created
GUA-PRESCTX-FBO status=0x8cd5
```

`0x8CD5` is `GL_FRAMEBUFFER_COMPLETE`. This proves the earlier `0x8CD7` was caused by the wrapped game-context framebuffer bind, not by an inherent inability of the second KMSDRM context to expose a drawable default framebuffer.

The game context still reaches an early non-black state:

```text
swap 3 game FBO nonblack=1952/6144
```

The presenter readback remains black in the current run, so the next narrow issue is presenter draw/resource state rather than default-FBO completeness. Existing game-side FBO errors may still occur later and must be separated from presenter errors.

## Status

This phase is partially successful: the concrete wrapped-FBO bug is fixed and verified. The two-context architecture remains viable and should not be discarded. No playable fix is claimed until the presenter-context draw produces non-black pixels and a synchronized physical/KMS capture confirms the same frame.

The latest candidate is intentionally left installed on TSPS. No production rollback was performed.
