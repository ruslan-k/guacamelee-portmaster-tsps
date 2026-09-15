# Cross-context texture handoff classifier — 2026-09-15

## Applied

Added a reversible diagnostic A/B controlled by `GUACAMELEE_PRESENT_UPLOAD=1`:

1. game context finishes rendering;
2. raw `glReadPixels()` captures the source FBO into a 1024x768 RGBA CPU buffer;
3. presenter context uploads the buffer into `game_color` with raw `glTexImage2D()`;
4. the presenter-local shader draws the texture.

This bypasses shared texture contents while preserving the same presenter drawable/program/VAO path. It is diagnostic-only and is not enabled by the launcher default.

## Device result

```text
game swap 10: nonblack=1952/6144
present upload: capture=1024x768 bytes=3145728
presenter swap 10: nonblack=0/6144

game swap 30: nonblack=1952/6144
present upload: capture=1024x768 bytes=3145728
presenter swap 30: nonblack=0/6144
```

No `GL_INVALID_*` or framebuffer-incomplete errors were observed in this run.

## Classification

The CPU handoff did not change the black presenter result. Therefore the failure is not merely stale/empty shared texture content. The remaining presenter boundary is either:

- texture upload/binding semantics in the presenter context;
- presenter shader input/object state despite a linked local program;
- or a cross-context/native GLES object operation not reported as an error.

The default drawable remains proven writable by the magenta classifier. Game FBO remains non-black. No game-side shader/FBO phase should be reopened.

The latest diagnostic candidate remains on TSPS; all test processes were stopped and MainUI remains active.
