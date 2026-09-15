# Geometry Phase 4 — SDL contract complete, application still letterboxes

## Device evidence

After the SDL geometry shim was deployed, the Guacamelee log reported:

```text
SDL_GL_GetDrawableSize 1280 x 720
SDL_GetWindowSize 1280 x 720
SDL_GetCurrentDisplayMode 1280 x 720
bridge_dimensions=1280x720
present stretch 1280x720 -> 1280x720
```

The mode-enumeration A/B also exposed one coherent display mode and did not produce FBO errors. Runtime reached the game loop and audio initialization:

```text
Total time until first game loop: 10.635824 secs
Audio 3D Memory
OnDraw: 2
```

## Physical captures

The physical KMS capture is 1280x720. Both splash and menu captures show a centered active image approximately 1280x540, with symmetric black regions of approximately 90px at the top and bottom. There is no right-side clipping or horizontal shift after the SDL window/drawable fix.

The menu is visible and the title screen is correct, but the application still does not fill the vertical scanout.

## Classification

The SDL geometry contract is now coherent. The remaining 1280x540 result is therefore inside Guacamelee's application resolution/render-target initialization, not presenter letterbox geometry or an SDL window-size mismatch.

A diagnostic presenter rebuilt with the host GCC was not deployed because its ELF/runtime closure failed the TSPS presenter-ready gate; the known-good presenter was restored immediately. No GL bridge or presenter source mutation remains from that experiment.

## Request for next plan

Please provide the next concrete, one-variable fix for the application-side 1280x540 render selection. In particular, identify where Guacamelee chooses the 1280x540 render target after SDL reports 1280x720, and specify whether to patch the resolution initialization, `SDL_SetWindowDisplayMode`, or a game-side setting. Do not change the proven presenter, shared-context, orientation, or gl4es path.
