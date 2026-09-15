# Remaining deterministic inputs and scratch sync classifier — 2026-09-15

## Full payload hashes

The bridge logged FNV-1a hashes over complete guest attribute uploads, not only
packet heads. For the close early pair, all 480-byte position/color/texcoord
uploads matched:

```text
position = b4ebc206553c1a23
colour   = 8596fc6a8f50e2b3
texcoord = 7df1902942837126
```

Later title progression uses different payload sizes and hashes, which are
expected scene data changes; they were not silently treated as equivalent.

No client-index upload was present for the captured DrawArrays pair. The
bridge's index staging path is separate and was not implicated in that pair.

## Scratch synchronization A/B

A diagnostic bridge called `glFinish()` immediately before late FBO1/program18
draws (`seq >= 7000`). The run still produced the same black destination:

```text
GUA-PRE-FINISH seq=7919 fb=1 program=18
FBO1 full readback after draw: nonblack=0/786432

GUA-PRE-FINISH seq=8031 fb=1 program=18
FBO1 full readback after draw: nonblack=0/786432
```

The one-shot synchronization classifier did not change the result. A simple
GPU completion or immediate scratch-buffer reuse stall is therefore not
confirmed.

## Final status

The remaining issue requires either a full native-vs-TSPS draw parity capture
at the matching scene state or deeper ARM GLES/Mali shader/input instrumentation.
No production workaround is justified from the current evidence.

The diagnostic bridge was removed and production runtime restored.
