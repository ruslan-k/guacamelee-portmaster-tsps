# Final remaining-phase integration run — 2026-09-15

## Applied

- made SDL offscreen normalization a launcher-managed x86 compatibility component;
- added and built `compat/libgua_sdl_mode_fix.so` as an i386 shared object;
- launcher defaults to `BOX86_LD_PRELOAD` of that shim and keeps opt-out `GUACAMELEE_SDL_MODE_FIX=0`;
- kept hardext preload disabled by default;
- kept true GLES2 request, reduced extension profile, VAO/EBO shadow changes, and client-array staging fix.

## Device prerequisite

The TSPS image has a mounted `/dev/shm`, but the directory was absent. Creating only `/dev/shm/portmaster` allowed the PortMaster launcher to proceed. No partition or mount change was made.

## Clean candidate run

The candidate was run with diagnostic flags disabled. It reached the game draw path:

```text
bridge_dimensions=1024x768
LIBGL_ES=2 LIBGL_GL=21 LIBGL_NOTEST=1 LIBGL_FBOFORCETEX=1 LIBGL_FB=1
hardext_fix=0
OnDraw: 2
OnDrawSub: 1
```

However, it still logged:

```text
GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT
GL_FRAMEBUFFER_UNSUPPORTED
glError (0x500)
```

Therefore the candidate is not a production fix. The separate presentation context phase remains unimplemented; this is the remaining architectural change needed before another clean run is meaningful.

## Restoration

Production was restored and read back:

```text
guacamelee_present 28668fbfc26bb4f207acefe436978923df6d545595ec7a16a0c58cdba1fb388
libEGL.so.1        f264443cf87daa740f0ad99de53292bcbcf07143e936cfc6d901efdc5f3e1be3
libGL.so.1         6b0c60b1f942a9a408d83bc6fdc46a44cbd7195aed482e7fe1cb43420f253253
game-bin           5aa2a2cc89d79912a4036ce5aca344fa6cc6f8767e346b8d271c3b76300aa063
```

MainUI is active and game/presenter/helper processes are absent.
