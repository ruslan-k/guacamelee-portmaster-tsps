# Guacamelee fullscreen physical capture

## Scope

Validate the TSPS Smart Pro S 1280x720 output after forcing SDL display geometry.

## Baseline capture

The first physical KMS capture showed the game content limited to approximately x=0..1019 and y=49..623, with a large right-side black region and asymmetric top/bottom borders. The launcher reported `present stretch 1280x720 -> 1280x720`, so this was not presenter letterbox mode.

The game log simultaneously reported:

```text
SDL_GetCurrentDisplayMode 1024 x 768
```

## Fix

The 32-bit SDL compatibility shim was changed to force 1280x720 for:

- `SDL_GetCurrentDisplayMode`;
- `SDL_GetDesktopDisplayMode`;
- `SDL_GetDisplayBounds`;
- `SDL_GetDisplayUsableBounds`.

The shim was built in an Ubuntu 20.04 container and installed as:

```text
c9cfffc314f143a7121665febd2f5c1d7dde8dff8918d0a87eb57ca5cec0215b
```

The launcher preload was corrected so the SDL shim is not overwritten by the evdev input shim.

## Device evidence

After the corrected deployment:

```text
SDL_GetCurrentDisplayMode 1280 x 720
Screenshot saved to /tmp/guac-boundsfix.png (FB ID: 165, 1280x720)
```

Visual inspection of the physical capture found:

- full 1280x720 frame;
- no distinct letterbox bars or side bars;
- no visible viewport displacement;
- dark splash content extends across the scanout.

The earlier screenshot `/tmp/guac-current.png` is retained locally as the before image; `/tmp/guac-boundsfix.png` is the after image.

## Request for review

Please review whether forcing SDL display bounds is the correct production fix for the Smart Pro S. In particular, confirm that the game should use the physical 1280x720 bounds rather than the offscreen backend's reported 1024x768 mode, and whether any game-side resolution setting should be changed instead.
