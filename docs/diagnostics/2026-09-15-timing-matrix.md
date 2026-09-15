# Box86 timing matrix — 2026-09-15

## Profiles

All three runs used the same production graphics profile and 20-second SSH
launch window. Diagnostics were enabled only for classification:

```text
LIBGL_NOTEST=1
LIBGL_ES=2
LIBGL_GL=21
LIBGL_FB=1
GUACAMELEE_GL_DIAG=1
GUACAMELEE_FBO_LIFECYCLE=1
GUACAMELEE_PIXEL_PROBE=1
GUACAMELEE_OP_RING=1
```

Profiles:

```text
D1: BOX86_DYNAREC=1 BOX86_LOG=0
D2: BOX86_DYNAREC=0 BOX86_LOG=0
D3: BOX86_DYNAREC=1 BOX86_LOG=1
```

## Results

### D1 — normal dynarec

- 2,934 log lines.
- Opened `misc.dat`, `levels.dat`, `resources.dat`.
- Parsed `preinit.ini`, `gameSettings.ini`, and `postinit.ini`.
- Reached repeated swap/draw traffic.
- Swap 3 produced the known non-black controller prompt signature:

```text
hash=675ae5fc56b53803
nonblack=2080/6144
min=0 max=255
```

- Later samples returned to the black signature.

### D2 — interpreter mode

- 1,126 log lines.
- Opened the three archives and parsed `preinit.ini` and `gameSettings.ini`.
- Did not reach `postinit.ini` or the normal repeated draw/swap progression in
  the 20-second window.
- No validated improvement; interpreter mode is not a production fix.

### D3 — dynarec plus `BOX86_LOG=1`

- 2,975 log lines.
- Reached the same archive/preinit/postinit milestones as D1.
- Reached repeated draw/swap traffic and the same controller-prompt signature:

```text
hash=675ae5fc56b53803
nonblack=2080/6144
min=0 max=255
```

- Later samples again returned to the black signature.

## Interpretation

The matrix confirms a timing/execution sensitivity in startup progression:

```text
D1 normal dynarec      -> full initialization and prompt frame
D2 interpreter         -> slower/earlier boundary, no full progression in 20s
D3 dynarec + logging   -> full initialization and prompt frame
```

`BOX86_LOG=1` is only an instrumentation perturbation and is not a safe fix.
The black-title boundary remains after the known prompt frame, so timing does
not by itself explain the later zero-content title frame.

All three runs were cleaned up. Production `libGL.so.1` was restored and
MainUI remained active.

Raw logs:

```text
/tmp/guacamelee-D1.log
/tmp/guacamelee-D2.log
/tmp/guacamelee-D3.log
```
