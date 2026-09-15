# Guest vertex-upload probe — 2026-09-15

The bridge was instrumented at `OP_glUploadAttrib` to log the actual guest
vertex blobs copied into server-side scratch VBOs, not merely buffer IDs and
strides.

For the adjacent GOOD/BAD program-18 draws, the real uploads matched:

```text
position  stride=20 bytes=480 head=000080bf0000803f
colour    stride=20 bytes=480 head=000000ff0000003f
texcoord  stride=20 bytes=480 head=0000003f0000003f
```

The source texture probe also remained stable:

```text
texture 3 nonblack=462/768 hash=204cbad35a34b548
```

Together with identical FBO/program/array metadata, this closes the practical
source-texture and guest-upload divergence branches for the captured pair.

The remaining failure is now a renderer/bridge state interaction that is not
explained by changed source pixels, changed vertex metadata, or changed guest
upload bytes. No production workaround was promoted. Diagnostic runtime was
removed and production hashes restored.
