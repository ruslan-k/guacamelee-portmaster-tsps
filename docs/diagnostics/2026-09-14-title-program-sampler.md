# Title fragment and sampler provenance

Date: 2026-09-14
Target: TrimUI Smart Pro / Spruce

## Proven state

With the coherent packed-D/S diagnostic profile:

```text
FBO2 = 0x8cd5 COMPLETE
DRAW_BUFFER0 = GL_COLOR_ATTACHMENT0
READ_BUFFER = GL_COLOR_ATTACHMENT0
viewport = 0,0,1024,768
scissor = disabled
color mask = 1111
```

The full FBO2 readback remained zero immediately after draw 16 and at swap 300:

```text
nonblack=0/786432
bbox=-1,-1..-1,-1
```

## Fragment passage

`GL_ANY_SAMPLES_PASSED` was queried around the first title draws:

```text
seq=11211 passed=1
seq=11280 passed=1
seq=11863 passed=0
seq=11870 passed=1
seq=11877 passed=1
seq=11884 passed=1
seq=11891 passed=1
seq=11898 passed=1
seq=11905 passed=0
seq=11912 passed=1
seq=11919 passed=1
seq=11926 passed=1
seq=11933 passed=1
```

Most title draws produce fragments. This is not a universal vertex/raster rejection.

## Program 2

The first title draw uses program 2. Its active uniforms are:

```text
sampler0 loc=0 type=0x8b5e value=0
WorldViewProj loc=1 type=0x8b5c value=1
```

The attached fragment shader is:

```glsl
uniform sampler2D sampler0;
varying vec4 v_texcoord0;
varying vec4 v_colour0;
void main()
{
    vec4 color = texture2D(sampler0, v_texcoord0.xy);
    gl_FragColor = v_colour0 * color;
}
```

Therefore a missing/empty/incorrect texture bound to texture unit 0 can produce black output without a GL error, even while occlusion queries pass.

The attached vertex shader uses:

```glsl
attribute vec4 in_position0;
attribute vec4 in_colour0;
attribute vec4 in_texcoord0;
uniform mat4 WorldViewProj;
varying vec4 v_colour0;
varying vec4 v_texcoord0;
```

## Interpretation

The evidence now points to program-2 sampler/texture state or the values of `v_colour0`/`v_texcoord0`. No shader rewrite has been applied. The next A/B should inspect or temporarily bind a known-good unit-0 texture only for program 2, while logging the original binding and texture level-0 state.

## Device state

Temporary diagnostic gl4es was restored:

```text
libGL.so.1 SHA-256:
6b0c60b1f942a9a408d83bc6fdc46a44cbd7195aed482e7fe1cb43420f253253
Guacamelee processes: none
```

Raw log: `2026-09-14-title-program-sampler.log.gz`
