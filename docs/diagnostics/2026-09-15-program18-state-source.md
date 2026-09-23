# Program-18 GOOD/BAD state and full-FBO probe — 2026-09-15

## Program identity

The dumped program-18 fragment shader is a textured multiply:

```glsl
vec4 color = texture2D(sampler0, v_texcoord0.xy);
gl_FragColor = v_colour0 * color;
```

The vertex shader passes `in_colour0` and `in_texcoord0` through and writes
`in_position0.xy` to clip-space position.

## State captured on FBO1

The good and bad program-18 draws matched in the captured fixed-function
state:

```text
FBO status       = 0x8cd5
DRAW_BUFFER0     = GL_COLOR_ATTACHMENT0
READ_BUFFER      = GL_COLOR_ATTACHMENT0
viewport         = 0,0,1024,768
scissor          = disabled
color mask       = 1111
blend            = enabled, equation ADD, SRC_ALPHA/ONE_MINUS_SRC_ALPHA
depth            = disabled, write disabled
stencil          = disabled
cull             = disabled
sampler unit 0   = texture 3
```

The draw arguments also remained ordinary non-indexed quad draws (`mode=0x4`,
counts 6 or 24 depending on the call). No FBO completeness or color-mask
failure was found at this boundary.

## Full destination readback

The full readback corrected the earlier local-sample interpretation and found
both partial and complete outcomes:

```text
seq=5182  program=18  nonblack=156010/786432
seq=5191  program=18  nonblack=17976/786432

seq=7120  program=18  nonblack=28492/786432
seq=7527  program=18  nonblack=0/786432

seq=7651  program=18  nonblack=161766/786432
seq=7660  program=18  nonblack=0/786432
```

The complete black frames have no nonblack bounding box and zero min/max.

## Current conclusion

Program 18 is not failing because of an obvious FBO, viewport, blend, depth,
or stencil state difference. The remaining high-value discriminator is the
source-side data for texture 3 and the dynamic vertex inputs:

```text
texture 3 pixels/parameters immediately before good and bad draws
in_colour0 values
in_texcoord0 values
position/VAO/EBO state
```

No workaround was promoted. Production presenter and bridge were restored
after the diagnostic run; MainUI remained active and no game processes remain.
