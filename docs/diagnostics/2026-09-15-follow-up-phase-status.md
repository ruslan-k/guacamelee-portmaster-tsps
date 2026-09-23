# Follow-up phase status — 2026-09-15

The requested phases were continued without stopping.

## Completed in this continuation

- read the new PR plan;
- completed keyed VAO shadow and VAO 0 lifecycle;
- added deterministic `/dev/shm/portmaster` preflight;
- added presenter letterbox viewport;
- created presenter-local shader program in `present_ctx`;
- moved presenter draw state to raw resolver pointers;
- ran magenta drawable classifier;
- ran CPU readback/upload texture handoff classifier;
- added tracked gl4es safe-profile patch and clean-source build script;
- extended offline invariant tests.

## Latest decisive result

The default presenter drawable is writable:

```text
presenter FBO status=0x8cd5
magenta clear: nonblack=4096/6144
```

The game FBO is non-black, but the texture draw remains black even after a CPU upload handoff:

```text
game swap 10: nonblack=1952/6144
present upload: 1024x768 RGBA, 3145728 bytes
presenter swap 10: nonblack=0/6144
```

This isolates the remaining failure to presenter texture binding/import/sampling or a native GLES object boundary. No new game-side FBO theory is justified.

## Build/deployment notes

- `python3 tools/test_port.py`: PASS;
- ARM bridge build: PASS;
- shell syntax and diff checks: PASS;
- latest candidate is installed on TSPS;
- MainUI is active and game/presenter/gptokeyb2 processes are stopped;
- no rollback was performed;
- fresh gl4es configure remains blocked because Atomic host lacks `libdrm-devel`, `mesa-libgbm-devel`, and `mesa-libEGL-devel`; `dnf` did not install them and no rpm-ostree reboot transaction was started.

This phase is diagnostic-only for `GUACAMELEE_PRESENT_UPLOAD`; the launcher default remains unchanged.
