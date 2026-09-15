# Guacamelee / TSPS — complete ChatGPT A/B matrix

Date: 2026-09-14
Target: TrimUI Smart Pro / Spruce, `192.168.50.135`

## Executive result

All actionable alternatives from the latest ChatGPT review were exercised or explicitly invalidated:

- current-head control with compatibility behavior disabled;
- minimal `GetHardwareExtensions()` capability shim;
- child-process `LD_PRELOAD` cleanup;
- direct pinned-gl4es `maxcolorattach=1`;
- gl4es-side renderbuffer size synchronization;
- gl4es-side viewport synchronization;
- separate depth/stencil formats;
- packed depth/stencil experiments;
- `LIBGL_FBOFORCETEX=0/1`;
- texture-storage fallback and frontend texture bookkeeping;
- real frontend/backend `glGetError` tracing;
- Box86 dynarec and signal/backtrace diagnostics;
- strace and xport request/reply tracing.

No tested combination produced a visible title/menu or gameplay frame. The last clean coherent gl4es run reached FMOD, `OnDraw`, and repeated swaps with server-side FBO workarounds disabled, but the physical frame remained black/fade. The remaining blocker is therefore below the already-proven transport/context/size layers: shader/render-state/texture content, final composition, or game scene state.

The device was restored after every gl4es A/B. Final device state:

```text
original libGL.so.1 SHA-256:
6b0c60b1f942a9a408d83bc6fdc46a44cbd7195aed482e7fe1cb43420f253253
Guacamelee processes: none
MainUI: running
```

## Fixed and independently verified boundaries

### Package and data

- The GOG Makeself wrapper was never executed.
- Its appended ZIP was extracted with forced ZIP mode.
- Game payload: 15 files, 590,380,840 bytes.
- Original `game-bin` SHA-256:
  `cfab463cab9f734bae0a9c102be89699c747ed0e51235e384eb02cff08d6b`.
- Original game data remains outside Git.

### ABI and bridge

The runtime path is:

```text
x86 game -> Box86 -> ARMHF gl4es -> ARMHF GLES proxy -> Unix socket -> AArch64 presenter -> Mali
```

The following are proven on the physical device:

- ARMHF loader, gl4es, proxy, and AArch64 presenter load successfully.
- Presenter SDL/Mali context creation succeeds.
- xport request/reply is complete.
- `GL_IMPLEMENTATION_COLOR_READ_FORMAT` (`0x8B9B`) returns `0x1908`.
- `GL_IMPLEMENTATION_COLOR_READ_TYPE` (`0x8B9A`) returns `0x1401`.
- Both synchronous calls show `enter -> lock-acquired -> sent -> reply-hdr -> end rc=0`.
- Presenter game FBO is complete at `1024x768`.
- The initial CPU-affinity failure was fixed fail-closed in setup: the verified five-byte call-site patch prevents binding to unavailable CPUs `0-3` on the TSPS `4-7` CPU set.

### Display dimensions

The GOG SDL build reports `1024x768` for both drawable and window size. The original bridge used `640x480`, which caused the observed splash displacement/clipping. The bridge, shared frame bounds, viewport/scissor defaults, and presenter were changed to `1024x768`.

Pixel evidence already published:

```text
640x480:  splash bbox x=758..1096, y=0..294; centroid about (927,118)
1024x768: splash bbox x=534..745, y=225..454; centroid about (639,326)
```

The 1024x768 splash is centered and not source-clipped. This issue is closed.

## A/B matrix

### A — current-head control, all new compatibility behavior off

Baseline:

```text
original PortMaster libGL.so.1
1024x768
LIBGL_ES=2
LIBGL_GL=21
LIBGL_NOTEST=1
LIBGL_FB=1
LIBGL_FBOFORCETEX=1
BOX86_DYNAREC=1
CPU-affinity patch enabled
HARDEXT_FIX=0
RB_ZERO_SIZE=0
FBO_TEXTURE_FALLBACK=0
UNIFY_DEPTH_STENCIL=0
RB_FORMAT_FIX=0
ZERO_VIEWPORT=0
REAL_GLERROR=0
```

Result:

- archives open;
- `preinit.ini`, `gameSettings.ini`, and `postinit.ini` complete;
- FMOD initializes;
- title/application draw and swap continue;
- FBO lifecycle shows missing/incomplete application attachments;
- physical frame: black/fade;
- game log contains `Unable to spawn P0. No spawn point`.

This reproduces the original black-title boundary.

### B — minimal hardext capability shim

Changed only `GetHardwareExtensions()` behavior:

```text
hardext.maxcolorattach: 0 -> 1
```

The minimal ARMHF shim calls the real gl4es function first, resolves the loaded gl4es handle explicitly, and clears `LD_PRELOAD` in its constructor:

```c
__attribute__((constructor))
static void clear_preload_for_children(void)
{
    unsetenv("LD_PRELOAD");
}
```

Proof marker:

```text
GUA-HARDEXT-ONLY notest=1 es=2 fbo=1 maxcolorattach=0->1 maxdrawbuffers=1
```

Result:

- the formerly rejected color attachment reaches the bridge;
- with the original PortMaster gl4es, the run reproducibly reaches `exit_code=134` before the title path;
- strace captured a Box86-thread fault:

```text
SIGSEGV, si_code=SEGV_MAPERR, si_addr=0x68
```

This route is diagnostic-only and is not a production fix. The constructor is present; the earlier inherited-`LD_PRELOAD` concern was addressed in the tested shim.

### C — direct pinned gl4es `maxcolorattach=1`

Pinned source:

```text
e6448b04e5bfdabffbd650e1ccc53b82cd8c1c5d
```

The source change was limited to the `LIBGL_NOTEST=1` capability path. The first source build had a GLX/SDL build-parity difference and stopped at:

```text
SDL_GetNumVideoDisplays() failed
GLX 1.2 and up are not supported
```

That artifact was rejected as a capability A/B because it changed the gl4es build surface. It was not treated as evidence against the hypothesis.

A later direct gl4es build with the PortMaster-compatible no-X11/EGL-wrapper profile remained in the draw loop, unlike the preload shim. It still produced no visible title frame.

### D — gl4es-side renderbuffer state synchronization

The correction was moved into `src/gl/framebuffers.c`, before gl4es writes `rend->width/height` and before it calls GLES:

```text
0x0 -> 1024x768
```

Physical markers:

```text
GUA-GL4ES rb-size 0x0 -> 1024x768 fmt=0x8058
GUA-GL4ES rb-size 0x0 -> 1024x768 fmt=0x88f0
```

This is the coherent version of the earlier server-side size override. It prevents:

```text
gl4es state: 0x0
Mali state: 1024x768
```

Result:

- process stays alive;
- archives, FMOD, `OnDraw`, and repeated swaps proceed;
- frame remains black/fade;
- by itself it does not provide a visible title.

The previous server-side `0x0 -> 1024x768` override was also tested and failed to change the frame; it is not a production solution.

### E — `LIBGL_FBOFORCETEX=0/1`

Both values were tested in the preserved baseline. The attachment type changes between renderbuffer and texture paths, but neither setting produces a visible title frame.

### F — depth/stencil handling

The normal separate formats were retained first:

```text
GL_STENCIL_INDEX8     = 0x8D48
GL_DEPTH_COMPONENT16  = 0x81A5
```

With synchronized sizes, the trace showed a transient unsupported/incomplete transition:

```text
FBO2 status = 0x8CDD
```

This demonstrated that matching dimensions alone does not prove the driver accepts the attachment arrangement.

The following invasive compatibility options were then tested:

```text
GUACAMELEE_UNIFY_DEPTH_STENCIL=1
GUACAMELEE_RB_FORMAT_FIX=1
```

They make the observed application FBO statuses complete, but the physical frame remains black/fade. They are not accepted as a final fix.

Important enum correction:

```text
0x8D48 = GL_STENCIL_INDEX8
0x8D53 = GL_RENDERBUFFER_ALPHA_SIZE
0x88F0 = GL_DEPTH24_STENCIL8
```

Therefore `0x8D48 -> 0x88F0` is a compatibility remap of a valid stencil format, not correction of an invalid alpha-size format. Existing diagnostics were corrected accordingly.

A separate frontend packed-depth/stencil experiment was repeated on the latest gl4es build with all server-side FBO workarounds off. No `GUA-GL4ES packed-ds` marker appeared, so the required path was not reached and this A/B is classified as invalid/no-op, not as evidence for or against packed storage.

### G — texture storage

The color texture path was tested in two forms.

#### Server fallback

The bridge-side fallback allocates missing color texture storage at `1024x768`:

```text
GUA-FBO texture fallback tex=2 -> 1024x768
```

Combined with the other server FBO workarounds, observed application FBOs become:

```text
FBO2 = 0x8CD5
FBO3 = 0x8CD5
FBO4 = 0x8CD5
```

The game remains alive but the frame remains black/fade. This proves only that the backend can be forced to accept the attachments.

#### Frontend bookkeeping

A clean gl4es-side `GL_FRAMEBUFFER_TEX_SIZE_FIX` was added for a texture with no storage and zero tracked dimensions. It updates gl4es bookkeeping and calls `glTexImage2D` at the same layer:

```text
GUA-GL4ES tex-size 0x0 -> 1024x768 tex=3 fmt=0x1908
```

Physical clean run settings:

```text
server RB_ZERO_SIZE=0
server FBO_TEXTURE_FALLBACK=0
server UNIFY_DEPTH_STENCIL=0
server RB_FORMAT_FIX=0
HARDEXT_FIX=0
LIBGL_FBOFORCETEX=1
GL4ES RB size fix=1
GL4ES texture size fix=1
GL4ES viewport fix=1
```

Result:

- frontend texture markers appear;
- application draws use `viewport=0,0,1024,768`;
- FBO2 has a transient `0x8CDD` transition, then FBO3–FBO6 report `0x8CD5`;
- no `GL_FRAMEBUFFER_INCOMPLETE` remains in the later loop;
- FMOD, `OnDraw`, and repeated swaps continue;
- physical frame remains black/fade.

This is the strongest clean state-coherence result, but it is not a visible-output fix.

### H — viewport

The zero viewport was moved to the same frontend layer as the gl4es state update:

```text
GUA-GL4ES viewport 0x0 -> 1024x768
```

Before the change, application FBO draws used:

```text
viewport=0,0,0,0
```

After the change:

```text
viewport=0,0,1024,768
```

FBO status and draw routing improve, but the physical frame remains black/fade.

### I — real frontend/backend GL errors

With opt-in real error forwarding and separate gl4es diagnostics:

```text
frontend gl4es error: 0x500 = GL_INVALID_ENUM
backend GLES error:  0x502 = GL_INVALID_OPERATION
```

The first error is created by gl4es `shim_error`, not by xport/Mali. The second is returned by the GLES backend. Both are reported at the game's `SetConstants()` check:

```text
../Libs/Graphics/VideoSys.cpp:1644
```

The frontend error source was instrumented with source/line markers. No error mode produced a visible title frame.

### J — Box86 and abort diagnostics

Tested:

```text
BOX86_DYNAREC=1/0
BOX86_DYNAREC_BIGBLOCK
BOX86_DYNAREC_STRONGMEM
BOX86_DYNAREC_SAFEFLAGS
BOX86_DYNAREC_WAIT
BOX86_LOG
BOX86_DLSYM_ERROR
BOX86_DYNAREC_LOG
BOX86_SHOWSEGV
BOX86_SHOWBT
BOX86_ROLLING_LOG
BOX86_JITGDB
```

Results:

- `DYNAREC=0` changes startup timing and reaches deeper initialization in some runs;
- `BOX86_LOG=1` is timing-sensitive;
- no tested Box86 setting produces a visible title;
- `SHOWBT`, `SHOWSEGV`, `ROLLING_LOG`, and `JITGDB` produced no usable symbolic backtrace;
- strace remains the useful signal-level artifact and captured `si_addr=0x68` on the preload-shim abort path.

### K — xport and presenter diagnostics

The bridge was instrumented at both ends. The complete request/reply sequence was observed for synchronous queries and for ongoing rendering. The presenter receives draw/swap traffic continuously. Therefore the black frame is not an xport deadlock.

The presenter-owned FBO remains complete and the application FBO draw calls reach the presenter. The missing visible output occurs after transport and context setup.

## Artifact hashes

Original shipped gl4es restored after all tests:

```text
6b0c60b1f942a9a408d83bc6fdc46a44cbd7195aed482e7fe1cb43420f253253
```

Diagnostic source-built gl4es artifacts:

```text
maxcolorattach-only diagnostic build:
c2210404cb35d287070bdf3fd1a7f252cefa7fb6702af95994c730423962b79f

internal RB-size build:
1dbed760794130ead70aef3d39a3e7b24426a29eb12bd247481804245ee39426

direct maxcolorattach + RB-size build:
c5d209f09427052510c1342f23089a9e58e341b05f6fb0490dc77f9753c97bc1

viewport diagnostic build:
6342cf8aff793553c1180ca5257c70f55b9176b403b0512d69fafb7a4b864ce5

error-source diagnostic build:
ecb1a930d0c770f2e17984d84bba33a7e69b9f74c54e181a7423dee11b49357a

clean frontend texture-bookkeeping build:
c778a0c17f9a0a9694bf36cd5a7b2517d1e07ad17fea61b1944ab2340f388fb8
```

Latest clean-run raw log:

- `2026-09-14-clean-coherent-fbo.log.gz`
- SHA-256: `0e66d4f8f6ce8d1c7bc9113b19556b29dc6bd5c5fcff11c7f9b1599f2b5a02a2`

Earlier raw artifacts already in this PR:

- `2026-09-14-proc-snapshot.txt.gz`
- `2026-09-14-baseline-strace.txt.gz`

## Final classification

ChatGPT's concrete FBO/GL4ES alternatives have been tested. None produced a visible title/menu or gameplay frame. The following are **not** production fixes:

```text
LD_PRELOAD hardext shim       -> reproducible pre-title fault path
server-side zero-size fix    -> no visual change; split-brain state
server texture fallback      -> backend complete, still black
format remap/unification     -> backend complete, still black
viewport fix                 -> correct viewport, still black
real glGetError              -> diagnostics only
packed DS A/B                -> invalid/no-op; marker absent
source-built GLX mismatch    -> rejected build-parity regression
```

The best remaining evidence is:

```text
FBOs and viewport can be made coherent;
OnDraw and swap continue;
physical DRM frame remains black/fade;
frontend GL_INVALID_ENUM and backend GL_INVALID_OPERATION occur in SetConstants();
normal game state reports no spawn point.
```

Further progress requires a new source of evidence or a different layer: capture actual shader program/uniform values and application/game-FBO pixel hashes, resolve the `SetConstants()` GL error call, or obtain a game-side crash/scene-state diagnostic. No additional ChatGPT FBO toggle is justified by the current evidence.
