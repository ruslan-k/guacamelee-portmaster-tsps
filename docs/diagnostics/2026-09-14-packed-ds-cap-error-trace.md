# Packed depth/stencil capability and exact FBO transition

Date: 2026-09-14
Target: TrimUI Smart Pro / Spruce

## Phase 1: exact transition

With the pinned diagnostic gl4es profile (`LIBGL_NOTEST=1`, clean gl4es RB-size/viewport fixes, server FBO workarounds disabled), FBO2 changed as follows:

```text
seq=5  glRenderbufferStorage                 0x0 -> 0x8cd7
seq=8  glFramebufferTexture2D(color tex=2)  0x8cd7 -> 0x8cd5
seq=19 glFramebufferRenderbuffer(
          GL_STENCIL_ATTACHMENT,
          GL_RENDERBUFFER,
          rb=4)                              0x8cd5 -> 0x8cdd
```

`0x8cdd` is `GL_FRAMEBUFFER_UNSUPPORTED`.

The attachment state at the failing transition was:

```text
color:   texture 2, tracked 1024x768
         secondary texture tracked 1024x768, n=1024x768
         source renderbuffer rb=2, 1024x768

depth:   renderbuffer 3, GL_DEPTH_COMPONENT16, 1024x768
stencil: renderbuffer 4, GL_STENCIL_INDEX8, 1024x768
```

Depth and stencil were distinct backend renderbuffers. The transition was not caused by a texture-size mismatch.

The original lifecycle diagnostic also queried `GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL` for renderbuffers and generated false KHR/error noise. The new trace avoids that query unless the attachment type is `GL_TEXTURE`.

## Phase 2: actual Mali format matrix

The presenter-side matrix ran on the real Mali GLES context and deleted all temporary objects afterward:

```text
M1  color=RGBA  depth=D16                 status=0x8cd6
M2  color=RGBA  depth=D16 + stencil=S8    status=0x8cdd
M3  color=RGBA  packed=D24S8 separate      status=0x8cd5
M3b color=RGBA  packed=D24S8 combined      status=0x8cd5
M4  color=RGBA4 depth=D16 + stencil=S8    status=0x8cdd
M5  color=RGBA4 packed=D24S8 separate     status=0x8cd5
M5b color=RGBA4 packed=D24S8 combined     status=0x8cd5
```

The matrix proves that the device supports packed D24S8 and rejects the separate D16+S8 combination in this context. The advertised extension is present:

```text
GL_OES_packed_depth_stencil
```

## Phase 3: capability-only A/B

Pinned gl4es was changed only to advertise the capability skipped by `LIBGL_NOTEST=1`:

```text
hardext.depthstencil: 0 -> 1
```

Marker:

```text
GUA-GL4ES packed-ds-cap depthstencil=0->1
```

Result:

```text
FBO2 status: 0x8cd5 -> 0x8cd5
KHR invalid-framebuffer messages: 0
```

The title still rendered all-zero pixels at swap 300:

```text
app    fb=2 nonblack=0/6144
 game  fb=1 nonblack=0/6144
window fb=0 nonblack=0/6144
```

Therefore the capability fix is a valid FBO fix, but not by itself a visual title fix.

## Post-FBO error trace

After removing diagnostic query contamination, the first real backend error was:

```text
seq=5246 op=glGenerateMipmap fb=1 program=2 error=0x502
```

KHR reports:

```text
glGenerateMipmap: <level> is not an accepted value
```

A targeted `LIBGL_AUTOMIPMAP=3` A/B did not remove this error or change the all-zero title pixels. Later `glBindFramebuffer` errors are downstream/repeated state errors, not the first trigger.

`SetConstants()` still reports `0x502`, but no direct `glUniform*` operation has yet been identified as the source. FBO2 is complete at this point, so the next investigation is texture/mipmap state and the shader/constant path.

## Device state

All temporary gl4es/presenter/launcher files were restored after testing:

```text
libGL.so.1 SHA-256:
6b0c60b1f942a9a408d83bc6fdc46a44cbd7195aed482e7fe1cb43420f253253
Guacamelee processes: none
```

Raw log: `2026-09-14-packed-ds-cap-error-trace.log.gz`
