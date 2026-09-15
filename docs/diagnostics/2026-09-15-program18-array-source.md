# Program-18 source/array probe — 2026-09-15

The final narrow array-state probe captured actual vertex attribute array
metadata for program 18, not only current generic attribute values.

At the adjacent draws:

```text
in_position0: size=2 type=GL_FLOAT stride=20 buffer=32
after/GOOD and BAD: enabled=1

in_colour0:   size=4 type=GL_UNSIGNED_BYTE stride=20 buffer=35
after/GOOD and BAD: enabled=1

in_texcoord0: size=2 type=GL_FLOAT stride=20 buffer=34
after/GOOD and BAD: enabled=1
```

The FBO, viewport, buffers, blend/depth/stencil state, texture unit, and array
metadata were stable across the captured GOOD/BAD program-18 draws. Full-FBO
readback still shows repeated transitions from nonblack to completely black
at later program-18 draws.

This closes the obvious VAO/stride/buffer divergence branch. The unresolved
source-side possibility is the actual contents/parameters of texture 3 or the
vertex data at the pointer offsets, which cannot be distinguished from the
current bridge-side metadata without a deeper buffer readback. No production
workaround is justified.

Production bridge/presenter restored after the run; patched `game-bin` and
production `libGL.so.1` hashes remain unchanged.
