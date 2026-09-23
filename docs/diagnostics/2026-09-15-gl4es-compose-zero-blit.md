# gl4es compose trace: zero-area BlitFramebuffer calls

Date: 2026-09-15
Target: TrimUI Smart Pro / SpruceOS

## Change

The frontend compose diagnostic originally let bind markers consume the shared 512-marker budget before the `glBlitFramebuffer` resolution path could be observed. The bind cap was reduced from 120 to 96 while retaining the 512-marker overall cap and all blit markers.

Diagnostic ARMHF `libGL.so.1`:

```text
SHA-256: 7ee790539e534f4013c253f65794cd7f682e2422c0224a64a6a616c06c49f6aa
```

The previous diagnostic library was backed up on-device at:

```text
/mnt/SDCARD/Roms/PORTS/guacamelee/backups/20260915-gl4es-compose-budget96/libGL.so.1.before
SHA-256: e716e27148711ac8328cc0641b742501d0752e8c3bbc55f48a02fc6dc47101e9
```

## Device run

The direct device-side run used the coherent frontend profile:

```text
GUACAMELEE_GL4ES_COMPOSE_DIAG=1
GL4ES RB-size / viewport / FBO-texture bookkeeping = 1
packed depth-stencil capability = 1
server-side FBO workarounds = 0
PRESENT_LAST_FBO = 0
```

The run reached 1,586 swaps. Parsed marker counts were:

```text
GUA-GL4ES-FB                 96
GUA-GL4ES-BLIT enter        416
GUA-GL4ES-BLIT resolved       0
```

All 416 `enter` records were zero-area calls:

```text
read=FBO2, draw=FBO3
src=0,0..0,0
dst=0,0..0,0
mask=GL_COLOR_BUFFER_BIT
filter=GL_LINEAR
```

In `gl4es_glBlitFramebuffer`, those calls return before texture resolution/composition because both source and destination extents are zero:

```c
if (dstX1 == dstX0 || dstY1 == dstY0) return;
if (srcX1 == srcX0 || srcY1 == srcY0) return;
```

## Forced-fullsize classifier

A separate opt-in A/B set `srcX1/srcY1` to the read FBO dimensions and
`dstX1/dstY1` to the draw FBO dimensions **only when all eight supplied
rectangle coordinates were zero**:

```text
GUACAMELEE_GL4ES_ZERO_BLIT_FULLSIZE=1
```

This is deliberately not a candidate fix. It was tested only to determine
whether the zero coordinates could be a lost fullscreen rectangle.

The 45-second device run reached 1,577 swaps. It proved the previously
unreached composition branch is viable:

```text
force-fullsize calls: 1,576
resolved markers:       208 (shared diagnostic marker budget)
read FBO2 / draw FBO3:  1024x768 -> 1024x768
logical texture:        2 (backend texture 2)
zoom:                   1.000,1.000
```

The game continued to submit zero rectangles on every call; the substitute
was therefore entirely frontend-invented. No visual result was accepted from
this SSH-launched diagnostic run, and the mode was not promoted. A
menu-launched, synchronized KMS capture would be required before any visual
claim.

## x86 resolver and call-site evidence

An i386 `BOX86_LD_PRELOAD` interposer exporting `glBlitFramebuffer` was loaded
as a separate trace boundary. Box86 explicitly reported loading it. The first
interposer did not see calls because the game resolves the symbol through
`glXGetProcAddressARB`.

A second interposer wrapped `glXGetProcAddressARB` and captured the complete
guest boundary:

```text
GUA-X86-GLXGETPROC call=0 name=glBlitFramebuffer -> shim
GUA-X86-GLXGETPROC call=1 name=glBlitFramebufferEXT -> shim
GUA-X86-BLIT call=0 src=0,0..0,0 dst=0,0..0,0 mask=0x4000 filter=0x2601
...
GUA-X86-BLIT call=4 src=0,0..0,0 dst=0,0..0,0 mask=0x4000 filter=0x2601
```

The x86 executable contains a direct `glXGetProcAddressARB` import and stores
the returned pointer in its renderer state. Static disassembly identifies the
resolver call for `glBlitFramebuffer` at guest VA `0x08927ff2`, with the
function pointer stored at `0x08c73df8`; the `EXT` alias is resolved at
`0x0891e94d` and stored at `0x08c72fb4`.

The relevant call-site is guest VA `0x0812d543`:

```asm
; edi = application render-state object
mov edx,[edi+0x8c]
mov eax,[edi+0x1c]
mov ecx,[edx+0x8]
mov edx,[edx+0xc]
mov [esp+0x18],ecx       ; dstX1
mov [esp+0x14],edx       ; dstY1
mov edx,[eax+0xc]
mov [esp+0x0c],edx       ; srcY1
mov eax,[eax+0x8]
mov [esp+0x08],eax       ; srcX1
mov [esp+0x04],0         ; srcY0
mov [esp+0x00],0         ; srcX0
mov [esp+0x10],0         ; dstX0
mov [esp+0x1c],0         ; dstY0
call [0x08c73df8]
```

Immediately before this, the game queries the read and draw framebuffer IDs:

```asm
mov [esp+4],0x8ca8       ; GL_READ_FRAMEBUFFER_BINDING
call glGetIntegerv
mov [esp+4],0x8ca9       ; GL_DRAW_FRAMEBUFFER_BINDING
call glGetIntegerv
```

The four non-zero rectangle values are read from application-owned metadata:

```text
read-side object:  [edi+0x1c], width=[+0x8], height=[+0xc]
draw-side object: [edi+0x8c], width=[+0x8], height=[+0xc]
```

A closer disassembly of the surrounding render-target setup changes the
interpretation of those fields. The object at `[edi+0x8c]` is not an
independent target object: it is assigned as a pointer to `[edi+0x94]` at
`0x0812ce44` / `0x0812d209`. The setup then writes the actual target size into
`[edi+0x9c]` and `[edi+0xa0]`, not into `[edi+0x94+0x8]` and
`[edi+0x94+0xc]`:

```asm
mov [edi+0x94], framebuffer_id
mov [edi+0x9c], width
mov [edi+0xa0], height
lea eax,[edi+0x94]
mov [edi+0x8c],eax
```

At the later blit site, however, the game reads `[edi+0x8c+0x8]` and
`[edi+0x8c+0xc]`, which are `[edi+0x9c]` and `[edi+0xa0]` for this alias and
therefore should be the populated size fields. A separate read-side pointer
at `[edi+0x1c]` is loaded earlier by the render-target selection path and can
be stale or zero-sized. Thus the earlier simplified statement that both
objects are independently zero-sized was too strong.

The new causal boundary is narrower: the draw-side dimensions are populated
by the setup path, while the read-side dimensions come from the pointer stored
at `[edi+0x1c]`. The next trace must identify that pointer's producer and
compare its `+0x8/+0xc` fields at the blit call. Do not patch gl4es or assume
both sides are invalid.

## Guest-pointer probe result

A second i386 preload was deployed with SHA-256:

```text
c55cdb7db5130aa4209d77b820fe73c4ab27ff4140af1b49bdf00b54da59c574
```

It logged the blit arguments and attempted to inspect the caller's native `edi`
register. The runtime output was:

```text
GUA-X86-BLIT call=0 edi=(nil) read=(nil) readwh=0,0 draw=(nil) drawwh=0,0
  src=0,0..0,0 dst=0,0..0,0
```

This does **not** prove the guest objects are null. The interposer executes
through Box86's host bridge; its host-side `edi` is not the guest i386 `edi`
from the application call-site. Therefore host-register dereferencing cannot
inspect guest memory and this probe is invalid for object provenance. It does,
however, reconfirm the five observed guest calls and their zero rectangles.

The correct next boundary is a Box86 guest-memory/register trace at the
interpreter call, or a binary patch that logs the guest objects before
`0x0812d543`. Do not use another host preload to read guest registers.

## Guest-aware i386 object probe

The latest recommendation explicitly required a guest-side probe rather than
reading the host ARM register state. That probe was implemented as a 32-bit
`BOX86_LD_PRELOAD` library with a naked assembly entry trampoline that captures
EDI before any compiler-generated register changes.

Probe SHA-256:

```text
5c9c0b8642f4c3d00df45b7a4641ec48935976e3b96aab91fe8839ef2d324f82
```

It resolved both symbols and captured the real guest state at the indirect
blit call:

```text
GUA-GUEST-PROBE resolve name=glBlitFramebuffer
GUA-GUEST-PROBE resolve name=glBlitFramebufferEXT
GUA-GUEST-PROBE call=0 edi=0x8c2ed20 read=0x8c2ed60 rw=0 rh=0 draw=0x8c2edb4 dw=0 dh=0
  src=0,0..0,0 dst=0,0..0,0 mask=0x4000 filter=0x2601
```

The same guest object addresses and zero dimensions were observed on calls
1--4. This is valid guest-side evidence; it supersedes the earlier invalid
host-register probe, whose host `edi` was `(nil)` because it ran across the
Box86 native bridge.

The relevant object relationships are now concrete:

```text
parent EDI              = 0x8c2ed20
read-side pointer       = [EDI+0x1c] = 0x8c2ed60 (EDI+0x40)
read width/height      = [EDI+0x48] / [EDI+0x4c] = 0 / 0
draw-side pointer      = [EDI+0x8c] = 0x8c2edb4
                   draw width/height = [EDI+0x9c] / [EDI+0xa0] = 0 / 0
```

This classifies the latest recommendation as case **B**: both source and
destination render-target metadata are zero in guest memory before the blit
argument construction. The zeroes are not introduced by gl4es or by the
interposer.

Static xrefs show the application has multiple writers for these fields,
including render-target setup at `0x0812ce32..0x0812ce4a` and another size
copy path at `0x08062a2d..0x08062a45`. The next causal step is therefore to
trace the earliest initialization of the shared cached dimensions, not to
rewrite the final blit or patch gl4es.

## First bad writer: global display dimensions are zeroed, not populated

The guest probe showed the two globals consumed by the later render-target setup
are both zero:

```text
[0x8c36704] = 0x00000000
[0x8c36708] = 0x00000000
```

Static analysis then found the relevant dataflow:

```asm
; 0x0816c285 -- global initialization path
fstp? / fst [0x8c36704] = 0.0
fst  [0x8c36708] = 0.0

; 0x081ed05f -- render-target setup
mov eax,[0x8c36704]
mov [ebx+0x14d8],eax
mov eax,[0x8c36708]
mov [ebx+0x14dc],eax
```

An exhaustive static search for stores to `0x8c36704` and `0x8c36708` found no
other non-zero writer in the binary. The later setup copies these zero globals
into the render-target metadata, and the final blit reads the resulting zero
width/height fields. SDL's independent `1024x768` results never reach these
application globals.

This is the current root boundary: the game-side display-dimension globals are
initialized to zero and have no observed runtime assignment from SDL/window
size. The production fix, if pursued, belongs at the application-side writer
or at the narrow initialization boundary represented by `0x0816c285`, not in
gl4es or the presenter.


```text
probe SHA-256: a05e0ecf9d840bbb5f5bcc568fe90277229868038c5432382c18aa7068dc0719
parent [EDI+0x14d8] = NULL
parent [EDI+0x9c]/[EDI+0xa0] = 0/0
```

Thus the render-target setup's expected metadata source is itself absent for
the object used by the final blit. This is stronger than merely observing
zero dimensions: the application has neither a valid cached target-size
object nor populated parent dimensions at this call. SDL still reports
1024x768, so the bad state is introduced between SDL initialization and the
render-target object setup.


Probe SHA-256:

```text
c4137a8dcc7f22a9a6e622f29613b795fccb1c82fc5b60b17a1c838c80f65afc
```

The exact game window returned correct dimensions at runtime:

```text
GUA-SIZE SDL_GL_GetDrawableSize window=0xb01150 w=1024 h=768
GUA-SIZE SDL_GetWindowSize window=0xb01150 w=1024 h=768
```

Therefore the zero values in the render-target metadata are not caused by
SDL reporting a zero window or drawable. The remaining bad writer is later in
the application's render-target object initialization/copy path. The size
probe was removed and production `libGL.so.1` remained unchanged.


```text
SHA-256: 5aa2a2cc89d79912a4036ce5aca344fa6cc6f8767e346b8d271c3b76300aa063
```

## Reversible guest metadata A/B

A diagnostic-only guest preload forced the missing application metadata to
`1024x768` immediately before the final blit and changed that blit's arguments
from zero-area to:

```text
src=0,0..1024,768 dst=0,0..1024,768
read/draw metadata = 1024x768
```

The preload was used only for this A/B; it did not alter production files.
With `GUACAMELEE_GL_DIAG=1`, `GUACAMELEE_FBO_LIFECYCLE=1`, pixel probing and
op-ring diagnostics enabled, the run showed:

```text
GUA-FBO bind-fb requested=0 actual=1
GUA-DRAW fb=1 ... viewport=0,0,1024,768
GUA-PIX swap=2 stage=game fb=1 size=1024x768 nonblack=0/6144 min=0 max=0
GUA-PIX swap=2 stage=window fb=0 size=1280x720 nonblack=0/6144 min=0 max=0
```

The A/B therefore repaired the guest compose routing enough to produce draws
on logical default/game FBO1, but it did **not** produce pixels. It also logged
`pure virtual method called`, so the forced metadata is not a valid production
fix and was rolled back. This distinguishes two boundaries:

1. zero application metadata prevents the intended final compose path;
2. after routing is forced, the source/content or remaining game render state is
   still black, so the zero metadata is not the sole black-frame cause.


The observed frontend `glBlitFramebuffer(FBO2 -> FBO3)` traffic is real. In
the normal path it is a legal no-op caused by zero source and destination
rectangles. The fullsize A/B proves FBO2 and FBO3 have coherent 1024x768
texture storage and gl4es can execute a copy when supplied dimensions, but it
does not prove that substitution is semantically correct.

The application owns the missing dimensions. The next causal question is
where the two application render-target objects are created/populated and why
`[object+0x8]` / `[object+0xc]` remain zero after the framebuffer IDs become
valid. Do not add another generic FBO or presenter workaround.

## Restoration

After the runs:

```text
production libGL.so.1 SHA-256:
6b0c60b1f942a9a408d83bc6fdc46a44cbd7195aed482e7fe1cb43420f253253
Guacamelee/game helper processes: none
MainUI PID 4154 remains active
```

Temporary x86 interposers were removed from the device after each probe.
Raw logs saved locally:

```text
/tmp/guacamelee-compose-budget96-real.log
/tmp/guacamelee-force-zero-blit-device.log
/tmp/guacamelee-x86-glxgetproc.log
```
