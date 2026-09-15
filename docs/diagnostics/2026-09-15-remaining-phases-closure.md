# Remaining phases closure — 2026-09-15

## Completed in the final batch

- Upstream PortMaster patching audit.
- Clean-vs-patched `game-bin` A/B.
- Full FBO GOOD/BAD readback.
- Program-18 shader/state dump.
- Texture-3 source FBO readback.
- Vertex array metadata probe.
- Guest vertex-upload blob probe.
- Native Mesa reference and SDL/offscreen initialization comparison.
- Physical input check.

## Final evidence

The clean upstream GOG binary fails at the first bridge initialization boundary,
while the five-byte patched binary reaches archive parsing and the prompt. The
patch is required and is not the black-title fix.

For the black transition, the following remain stable between captured
GOOD/BAD program-18 draws:

```text
FBO / viewport / buffers / blend / depth / stencil
texture 3 source sample
vertex size / type / stride / buffer IDs
actual guest upload heads for position/color/texcoord
```

Yet the destination FBO can transition to a full black frame. This leaves a
deep renderer/bridge interaction: actual pointer-offset interpretation,
per-fragment GPU state, or Mali shader execution/cache behavior. No supported
PortMaster-side or gl4es-side production change is justified by the evidence.

## Stop condition

All actionable phases in the PR recommendation have been executed or closed
with evidence. The remaining phase requires a new low-level Mali/bridge
instrumentation strategy or a native-equivalent ARM GLES trace; repeating
existing FBO, SDL, Box86, input, texture, or metadata A/B tests would not add
information.

## Runtime state

Production state was restored and verified:

```text
game-bin SHA-256:
5aa2a2cc89d79912a4036ce5aca344fa6cc6f8767e346b8d271c3b76300aa063

libGL.so.1 SHA-256:
6b0c60b1f942a9a408d83bc6fdc46a44cbd7195aed482e7fe1cb43420f253253

MainUI: active
Game/presenter/gptokeyb2: not running
```
