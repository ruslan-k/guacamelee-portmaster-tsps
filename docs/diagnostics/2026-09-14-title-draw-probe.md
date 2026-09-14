# Title draw probe after packed depth/stencil fix

Date: 2026-09-14
Target: TrimUI Smart Pro / Spruce

## Baseline used

The tested diagnostic profile was:

```text
pinned gl4es
hardext.depthstencil=1
GL4ES RB-size fix=1
GL4ES texture bookkeeping fix=1
GL4ES viewport fix=1
LIBGL_FBOFORCETEX=1
server-side FBO workarounds=0
```

The `hardext.depthstencil=1` capability is supported by the real Mali matrix and keeps the game FBO valid.

## Mipmap provenance and narrow A/B

The first auto-mipmap call was initially:

```text
tex=3 size=1024x768 valid=0 mipmap_need=1
```

This was a FBO-created placeholder whose backend storage existed but whose gl4es metadata was still invalid. The gl4es FBO texture bookkeeping fix now records it as:

```text
GUA-GL4ES fbo-tex-bookkeeping tex=3 valid=1 size=1024x768 base=0 max=0 mipmap_need=0 min=0x2601
```

The next proven mipmap calls were:

```text
tex=4 size=32x32 RGBA valid=1 min=GL_LINEAR_MIPMAP_NEAREST
tex=0 size=0x0 valid=0
```

A narrow `GUACAMELEE_SKIP_BAD_MIPMAP=1` A/B skipped only the valid 32x32 signature and undefined texture metadata:

```text
GUA-MIP-SKIP tex=4 size=32x32 reason=valid-pot-rgba-generate-error
GUA-MIP-SKIP tex=0 size=0x0 reason=undefined-texture
```

The mipmap KHR error disappeared. `SetConstants()` errors decreased from five to four, proving that at least one earlier error was sticky error state. Title pixels remained zero, so mipmaps were not the visual blocker.

## First-title-draw probe

The probe read a 16x16 RGBA sample immediately after each of the first 16 draw calls while FBO2 was bound and complete:

```text
GUA-TITLE-PIX draw=0 seq=11328 op=DrawArrays   fb=2 nonblack=0/768
GUA-TITLE-PIX draw=1 seq=11388 op=DrawArrays   fb=2 nonblack=0/768
GUA-TITLE-PIX draw=2 seq=11971 op=DrawElements fb=2 nonblack=0/768
GUA-TITLE-PIX draw=3 seq=11978 op=DrawElements fb=2 nonblack=0/768
...
GUA-TITLE-PIX draw=15 seq=12062 op=DrawElements fb=2 nonblack=0/768
```

The same run recorded:

```text
FBO2 status=0x8cd5
KHR invalid-framebuffer messages=0
```

At swap 300:

```text
app    fb=2 nonblack=0/6144
game  fb=1 nonblack=0/6144
window fb=0 nonblack=0/6144
```

Therefore the title is already colorless immediately after its first draw, not erased by a later pass. The remaining blocker is now program/sampler/vertex-input state: shader output, bound textures, uniforms, or vertex attributes.

## Device state

The diagnostic gl4es library, presenter and launcher were restored after the run:

```text
libGL.so.1 SHA-256:
6b0c60b1f942a9a408d83bc6fdc46a44cbd7195aed482e7fe1cb43420f253253
Guacamelee processes: none
```

Raw log: `2026-09-14-title-draw-probe.log.gz`
