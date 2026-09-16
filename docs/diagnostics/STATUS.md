# Box32 direct rendering — current status

Updated 2026-09-17. Later reports supersede only the specific overclaims noted below; raw historical reports remain preserved.

## Current classified state

- Box32 + SDL2 + AArch64 gl4es launch: PASS
- native KMSDRM/EGL context and real draws/swaps: PASS
- natural program2 non-black raster control: PASS
- program18 first black writer: DISPROVEN
- program86 visible `NONBLACK -> BLACK` writer: PROVEN
- P86 sampler0/FBO2 texture object match: YES
- texture2 generation alive; delete/reuse mismatch: NOT OBSERVED
- FBO1 source immediately before resolve: BLACK
- FBO2 destination immediately after resolve: BLACK
- scene selected draws: native/default FBO0
- resolve source: FBO1
- generic split READ/DRAW control: PASS
- native error pre-clean window: prior 0x502 exists before P22 matrix
- first direct draw checkpoint with 0x502: program2/glDrawArrays
- P22 matrix direct producer: NO
- stable visible menu/gameplay: NOT PROVEN
- input/audio/normal PortMaster lifecycle: NOT TESTED

## Explicitly closed branches

Do not reopen X11, ARMHF bridge, presenter, global FBO remapping,
LIBGL_FBOFORCETEX, forced vertex colors, shader rewrites, paid-binary patches,
input or audio until a stable visible frame exists.

## Historical correction

`NATIVE_502_PRODUCER=glUniformMatrix4fv` from earlier post-call-only evidence is
superseded. The corrected pre-clean window found a pending native error before the
matrix call; the bounded post-draw checkpoint found the first non-zero error after
program2 `glDrawArrays`.

## Next unresolved boundary

Determine why program2/native scene rendering and the resolve path leave both FBO0
and FBO1 black at the exact pre-resolve boundary, and whether the native draw
operation itself is being rejected for a reason not exposed by the current error
trace. No fix is selected yet.
