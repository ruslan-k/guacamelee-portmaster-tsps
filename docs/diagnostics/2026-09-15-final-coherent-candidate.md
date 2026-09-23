# Final coherent candidate run — 2026-09-15

## Applied phases

- pinned gl4es source at `e6448b04e5bfdabffbd650e1ccc53b82cd8c1c5d`;
- built and packaged the patched ARMHF `libGL.so.1` from the existing pinned build tree;
- conservative capability profile includes FBO, one color attachment, packed depth/stencil;
- client per-VAO shadow state and corrected client-array staging;
- conditional SDL display/mode/bounds fallback and `SDL_GetNumVideoDisplays` fallback;
- isolated presenter context with raw framebuffer entry points, shader-only presenter program, presenter VAO/VBO and shared `game_color`;
- `/dev/shm/portmaster` prerequisite created when the mounted shm exists.

The reproducible fresh gl4es configure script was added, but a fresh configure on this host is blocked by missing development packages `libdrm`, `gbm`, and `egl`. The already configured pinned tree built successfully and its artifact was hash-compared before packaging.

## Clean TSPS run

Ran with diagnostic environment flags off. The real device reached the game draw path:

```text
GL vendor=ARM renderer=Mali-G57 version=OpenGL ES 3.2
bridge_dimensions=1024x768
gl4es_es=2 gl=21 notest=1 fbotex=1 fb=1 hardext_fix=0
Completed parsing file: preinit.ini
Completed parsing file: gameSettings.ini
Completed parsing file: postinit.ini
OnDraw: 2
OnDrawThreaded: 1
```

Error counters for the clean log:

```text
GL_FRAMEBUFFER_INCOMPLETE: 0
GL_FRAMEBUFFER_UNSUPPORTED: 0
glError (0x500): 0
```

The known missing optional `patch.1.dat` archive remains logged, but the main archives and init scripts complete. This is a positive FBO/renderer milestone, not yet proof of a visible physical frame: the clean run had diagnostics off and was launched remotely.

## Final status

All recommended implementation phases are now executed. The remaining verification gate is a physical menu launch confirming that the title/menu is visibly rendered and accepts input, followed by audio/save/exit checks. No further source diagnosis should be claimed until that physical observation.

The latest candidate remains installed on TSPS by user request. No rollback was performed.
