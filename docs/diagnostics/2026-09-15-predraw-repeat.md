# Repeated PRE-draw proof and restoration — 2026-09-15

A second authorized PRE-draw run reproduced the earlier result:

```text
seq=7919  FBO1 PRE  nonblack=0/786432
seq=7919  FBO1 POST nonblack=0/786432

seq=8031  FBO1 PRE  nonblack=0/786432
```

Texture 3 remained nonblack and stable. These late program-18 draws receive an
already-black FBO1 and are not the root writer.

The attempted tiny-op environment flag did not produce `GUA-TINY-OP` lines
through the launcher/server boundary, so no claims are made from that missing
trace. The next trace must enable the interval diagnostic at the server's own
configuration boundary or use an explicitly exported launcher variable.

Production was restored from `portmaster/dist/guacamelee.zip` and verified:

```text
guacamelee_present:
28668fbfc26bb4f207acefe436978923df6d545595ec7a16a0c58cdba1fb388

libEGL.so.1:
f264443cf87daa740f0ad99de53292bcbcf07143e936cfc6d901efdc5f3e1be3

libGL.so.1:
6b0c60b1f942a9a408d83bc6fdc46a44cbd7195aed482e7fe1cb43420f253253

game-bin:
5aa2a2cc89d79912a4036ce5aca344fa6cc6f8767e346b8d271c3b76300aa063
```
