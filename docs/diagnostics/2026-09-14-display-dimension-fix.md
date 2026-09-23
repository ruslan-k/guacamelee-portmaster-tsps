# Display dimension A/B — 2026-09-14

## Finding

The GOG SDL build reports a drawable/window of `1024x768`:

```text
SDL_GL_GetDrawableSize 1024 x 768
SDL_GetWindowSize 1024 x 768
```

The old bridge default was `640x480`. The game kept issuing viewport-space coordinates for its 1024x768 drawable; the bridge clipped them to the smaller FBO before letterboxing. This produced a splash shifted to the upper-right and clipped at the top.

## Pixel evidence

Both runs used the same patched game executable, presenter, gl4es, Box86 settings, and 1280x720 DRM capture. Only the bridge dimensions changed.

Old `640x480` capture (`rapid640-t14.png`):

- bright splash bbox: `x=758..1096`, `y=0..294`
- pixel centroid: approximately `(926.8, 117.6)`
- expected 4:3 letterbox content region: `x=160..1119`
- result: content is approximately `+288 px` to the right and clipped at the top

Corrected `1024x768` capture (`rapid1024-t14.png` / production `rapidfinal-t14.png`):

- bright splash bbox: `x=534..745`, `y=225..454`
- pixel centroid: approximately `(639.3, 326.2)`
- result: centered horizontally; no source clipping

The later corrected frame had the expected pillarbox region `x=160..1119` and centroid approximately `(639.6, 359.6)`.

## Implemented fix

- production launcher defaults: `1024x768`
- `TSPGL_WIDTH/HEIGHT` are exported after the Guacamelee defaults are established
- shared frame maximum height: `768`
- server fallback/default dimensions: `1024x768`
- EGL fallback dimensions: `1024x768`
- xport initial viewport/scissor dimensions: `1024x768`
- bounded FBO diagnostic markers remain opt-in under `GUACAMELEE_GL_DIAG=1`
- `README.md` and offline contract tests updated

The `640x480` mode remains available as an explicit reversible A/B via:

```text
GUACAMELEE_WIDTH=640 GUACAMELEE_HEIGHT=480
```

## Device verification

Production-default run on TSPS:

```text
bridge_dimensions=1024x768
SDL_GL_GetDrawableSize 1024 x 768
SDL_GetWindowSize 1024 x 768
Total time until first game loop: 13.918034 secs
cleanup_active []
mainui ['2337']
```

Splash frames were present at `rapidfinal-t13..t21`. No Guacamelee process remained after cleanup, and MainUI was running again.

The run still logs pre-existing/investigated FBO completeness warnings and one `GL_INVALID_ENUM` report; these are separate from the proven dimension/shift defect because the corrected splash and later centered 4:3 frame are visible.
