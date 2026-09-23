# Church HUD crop investigation — 2026-09-23

## Evidence

The user’s two reference images show the expected health/life indicator in a black top HUD strip. The crop-enabled physical church scanout is 1280x720 (`diagnostics-run/20260923/guac-church-current.png`, SHA-256 `6d684159b3b6e8bed91ce4d1332b7def4cbb233f43c516fb74477c1d7b149504`); the indicator is absent.

The presenter uses `v0=90/720`, `v1=630/720` for 1280x720, so source rows 0–89 (where the HUD is expected) are discarded. This is a strong mechanism-level hypothesis, not yet proof of the sole cause.

## Crop-disabled A/B

The active device launcher was backed up and temporarily set to `GUACAMELEE_PRESENT_SOURCE_CROP_90=0`. Its physical KMS capture (`diagnostics-run/20260923/guac-church-crop0.png`, SHA-256 `a2f86f5ea216a6fa865843e90be1e6e4cf0124990c9031fd54c32690fef40912`) showed the Guacamelee logo splash, not the church/HUD, so this is not a valid same-scene A/B. The original crop-enabled launcher was restored and its SHA-256 verified as `b1d0b3639f56e27d4b42a5fad382182f8e298070fb0bf2dfcd7b25eeef9249cc`.

The existing full-frame PPM checkpoints are swaps 10 and 600 and contain an earlier splash/menu, not the church frame. They cannot localize the current scene’s clipping.

## Bounded scene-triggered capture

`GUACAMELEE_PIXEL_DUMP_TRIGGER_FILE` adds a one-shot, same-swap capture. When both `GUACAMELEE_PIXEL_PROBE=1` and `GUACAMELEE_PIXEL_DUMP=1`, the presenter polls the trigger path every 30 swaps. On detection it removes the trigger and dumps the existing `app`, `game`, `present-source`, `present-context`, and `window` stages at that swap, using the existing GL state save/restore path. It is opt-in by default.

For the next run, enable the two flags and set the trigger to `/tmp/tspgl-pixel-dump.trigger`. When the church and affected HUD are visible, create the trigger once, capture physical KMS, then promptly copy/hash/inspect the same-swap PPMs. Do not change crop bounds until that evidence is available; retain symmetric crop as rollback.

## Cleanup

The crop-disabled test game was stopped by exact PID. No game, presenter, or Xbox helper remained, and SpruceOS MainUI was visually verified (`diagnostics-run/20260923/mainui-after-crop-ab.png`, SHA-256 `2fb7820c92f4cb496a8f8004e9bc2f2bd2b5f43fec66172076a0ff4bb827eadc`).
