# gl4es `maxcolorattach=1` A/B — 2026-09-14

## Purpose

Test ChatGPT's focused hypothesis: with `LIBGL_NOTEST=1`, gl4es' conservative early-return path leaves `hardext.maxcolorattach` at its zero-initialized value. Add only `hardext.maxcolorattach = 1` to that path and run the existing TSPS baseline.

## Preserved baseline

```text
1024x768
LIBGL_ES=2
LIBGL_GL=21
LIBGL_NOTEST=1
LIBGL_FBOFORCETEX=1
LIBGL_FB=1
DYNAREC=1
BOX86_LOG=0
CPU-affinity patch enabled
GUACAMELEE_FBO_LIFECYCLE=1
GUACAMELEE_GL_DIAG=1
```

The production `libGL.so.1` was backed up before replacement:

```text
backup: /home/ruslan/guacamelee-backups/20260914-gl4es-maxcolorattach-before/libGL.so.1
original SHA-256: 6b0c60b1f942a9a408d83bc6fdc46a44cbd7195aed482e7fe1cb43420f253253
```

The replacement was built from pinned upstream gl4es commit:

```text
e6448b04e5bfdabffbd650e1ccc53b82cd8c1c5d
```

Source change was limited to:

```c
hardext.maxcolorattach = 1;
```

plus a proof marker:

```text
GUA-GL4ES notest fbo=1 maxcolorattach=1 maxdrawbuffers=1
```

Replacement SHA-256:

```text
c2210404cb35d287070bdf3fd1a7f252cefa7fb6702af95994c730423962b79f
```

ELF verification passed: ARM ELF32, EABI5, hard-float ABI. The source build is not stripped and is therefore a diagnostic A/B artifact, not yet a production package.

## Physical result

The marker appeared from the library actually loaded by Box86:

```text
GUA-GL4ES notest fbo=1 maxcolorattach=1 maxdrawbuffers=1
```

The game advanced beyond the former archive/preinit boundary:

```text
Opened archive 'misc.dat' [76 files]
Opened archive 'levels.dat' [43 files]
Opened archive 'resources.dat' [2045 files]
Completed parsing file: preinit.ini
Loading shader cache...success!
```

However, no `GUA-FBO attachment`, `GUA-DRAW`, or `GUA-SWAP` marker appeared after the maxcolorattach marker. The game then shut down cleanly at the same early video initialization boundary:

```text
SDL_GetNumVideoDisplays() failed
GLX 1.2 and up are not supported
Prepping shutdown...
Shutting down...
exit_code=0
```

This run therefore proves that the one-field gl4es patch is loaded and changes initialization progress, but it does **not** yet prove a title-FBO fix. It did not reach the title-scene draw path in this invocation.

## Cleanup

The replacement library was reverted after the run. Device read-back is again:

```text
6b0c60b1f942a9a408d83bc6fdc46a44cbd7195aed482e7fe1cb43420f253253
```

Exact Guacamelee processes are gone and MainUI is running (PID 4573). No production source or tracked binary was changed by this A/B.

## Conclusion

`hardext.maxcolorattach = 1` is a real, minimal gl4es capability correction: the marker proves the conservative path no longer reports an impossible zero color-attachment capability. But this particular run ended before FBO creation/title rendering, so the hypothesis is not confirmed as the black-title fix. The next run must preserve the patched library and investigate why this source-built gl4es variant reaches `SDL_GetNumVideoDisplays`/GLX shutdown instead of the previously observed title path. Do not replace the production binary or merge this A/B yet.
