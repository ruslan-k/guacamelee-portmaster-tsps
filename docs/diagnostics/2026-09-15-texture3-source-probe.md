# Texture-3 source probe — 2026-09-15

A temporary diagnostic FBO attached texture 3 and read a 16x16 source sample
for each captured program-18 draw. The original framebuffer binding was
restored after every probe.

Texture 3 remained nonblack and unchanged across the observed draws:

```text
seq=5254  texture=3  nonblack=462/768  hash=204cbad35a34b548
seq=6078  texture=3  nonblack=462/768  hash=204cbad35a34b548
seq=6584  texture=3  nonblack=462/768  hash=204cbad35a34b548
```

The destination FBO1 contained visible content after the corresponding draw
but later became black while the texture-3 source sample remained identical.

Combined with the array probe:

```text
position: size=2 float stride=20 buffer=32
colour:   size=4 ubyte stride=20 buffer=35
texcoord: size=2 float stride=20 buffer=34
```

this closes the simple “source texture became black” branch and the obvious
array metadata branch. The remaining unresolved data is the actual contents at
the vertex-buffer pointer offsets and the exact per-draw vertex/color values
reaching the shader. No production workaround is justified.

Diagnostic bridge/presenter restored after the run. Production hashes remain
unchanged.
