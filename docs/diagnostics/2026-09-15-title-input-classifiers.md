# Title input classifiers and presentation routing

Date: 2026-09-15
Target: TrimUI Smart Pro / Spruce

## Diagnostic safety correction

The previous program dump incorrectly passed a one-element `int32_t` to `glGetUniformiv()` for all uniform types. Program 2 contains a `mat4`, so this could overwrite the presenter stack. The dump now uses bounded buffers and reads only sampler/integer values with `glGetUniformiv()` and float/vector/matrix values with `glGetUniformfv()`.

A clean rerun after this correction reproduced normal presenter operation and full readbacks.

## Presentation routing A/B

At swap 300 the last application draw target was FBO2, while the presenter’s fixed source was game FBO1:

```text
last-draw-fb=2 game-fbo=1
FBO2 full readback: nonblack=63985/786432 bbox=196,215..807,487
FBO1 readback: nonblack=0 in the sampled regions
```

An opt-in routing mode, `GUACAMELEE_PRESENT_LAST_FBO=1`, selected FBO2 as the blit source. The corresponding window framebuffer became non-black:

```text
present-source fb=2 nonblack=2583/6144
window fb=0 nonblack=2535/6144
window full readback nonblack=56403/921600 bbox=344,202..916,457
```

The first synchronized held KMS capture at swap 300 showed the Drinkbox Studios splash logo. This proves that selecting the last application FBO can present that intermediate frame physically. It is not yet a complete game fix: by swap 500 the source FBO2 itself was black and the held KMS frame was black/fade.

`TSPGL_PRESENT_SET_READ_BUFFER=1` was also tested. The source read buffer was already `GL_COLOR_ATTACHMENT0` (`old=0x8ce0`), and this A/B did not change the result.

## Program 2 input classifiers

Program 2 uses:

```text
sampler0 -> texture unit 0
unit 0 original binding = texture 4
```

Its fragment shader samples `sampler0` and multiplies by `v_colour0`.

Two bounded classifiers were tested only for `FBO2 + program2`:

1. `GUACAMELEE_TITLE_WHITE_TEX=1`: bind a 1x1 opaque white texture to unit 0, then restore the original binding and active unit.
2. `GUACAMELEE_TITLE_WHITE_COLOR=1`: disable only `in_colour0`, set generic `(1,1,1,1)`, then restore enabled and generic state.

Both markers were observed, but neither changed the full FBO result or the physical frame. The combined white-texture + white-color run also did not change the result. These are classifiers, not production fixes, and remain default-off.

## Program 49 / fade evidence

Program 49’s attached fragment shader is:

```glsl
gl_FragColor = vMult + vAdd;
```

The captured values were:

```text
vMult = 0,0,0,1
vAdd  = 0,0,0,0
```

This explains the later black/fade output at the shader level. At swap 500 the source FBO2 was independently confirmed all-zero, so the presenter was not losing a non-black frame at that point.

## Invalid classifier removed

A solid-magenta classifier was started but not accepted: its physical run caused the presenter to segfault before FBO2/title traffic, so no result was interpreted. The classifier and its launcher/test exports were removed from the branch and were not deployed as a fix.

## Device restoration

After the tests:

```text
libGL.so.1 = 6b0c60b1f942a9a408d83bc6fdc46a44cbd7195aed482e7fe1cb43420f253253
presenter = previously checked build without magenta classifier
Guacamelee/game helper processes = none
```

Raw classifier log: `2026-09-15-title-input-classifiers.log.gz`.

## Current interpretation

The original FBO2→window routing issue is real for the intermediate Drinkbox frame, but it does not explain the later black state because FBO2 itself becomes zero by swap 500. The remaining blocker is likely the game’s title/fade state or the values/updates driving program 49 (`vMult`/`vAdd`), not a generic sampler, color-write, FBO, or KMS presentation failure.
