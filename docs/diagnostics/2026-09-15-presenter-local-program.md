# Presenter-local program classifier — 2026-09-15

## Applied

- keyed per-VAO shadow table expanded to 128 entries;
- VAO 0 initialized explicitly and restored on deletion of the current VAO;
- deterministic `/dev/shm/portmaster` launcher preflight;
- letterbox viewport uses calculated `dx,dy,dw,dh`;
- presenter-local `present_prog` is created while `present_ctx` is current;
- presenter draw state uses raw resolver pointers for shader, VBO, VAO, texture, viewport and state operations;
- bounded magenta clear classifier retained behind `GUACAMELEE_PRESENTER_MAGENTA`.

## Device evidence

Offline gates:

```text
python3 tools/test_port.py: PASS
ARM bridge build: PASS
bash -n launcher/build scripts: PASS
git diff --check: PASS
```

With magenta classifier enabled:

```text
presenter default FBO: 0x8cd5 (COMPLETE)
presenter readback after clear/draw: nonblack=4096/6144
```

This proves the presenter default drawable is writable. With magenta disabled and the presenter-local shader program:

```text
present_prog=3, pos=0, uv=1
game swap 10: nonblack=1952/6144
presenter swap 10: nonblack=0/6144
game swap 30: nonblack=1952/6144
presenter swap 30: nonblack=0/6144
GL_FRAMEBUFFER_INCOMPLETE: 0
GL_FRAMEBUFFER_UNSUPPORTED: 0
```

Therefore the remaining defect is isolated to cross-context texture availability/sampling or the shared texture's content, not default-FB completeness, game FBO rendering, or the old wrapped framebuffer bind.

The latest candidate remains installed on TSPS. No rollback was performed. MainUI remains active and test processes were terminated.

## Not changed

`tspgl_stage()` was not changed because returning NULL on allocation failure requires auditing every asynchronous upload caller; blindly changing it would create a new crash path. This is a separate robustness phase, not evidence for the current black frame.
