# Guacamelee TSPS: source-FBO letterbox fix (2026-09-23)

## Result

The user confirmed that the game now displays correctly and audio is present. A physical TSPS framebuffer capture was taken during gameplay at 1280×720 (SHA-256 `37331427e0cde2611124b648f3cacc6320f96e6cca7622f07e23cfbbb44e5d0d`). The active game environment had `GUACAMELEE_PRESENT_SOURCE_CROP_90=1`; the presenter logged:

```text
GUA-PRES-SOURCE-CROP90 active src=1280x720 v=0.125..0.875 dst=1280x720
```

The game log reports `FMOD init succeeded with driver 0`; the user independently confirmed audible sound.

## Evidence locating the bars

After enabling `GUACAMELEE_GL_DIAG=1` and `GUACAMELEE_FBO_LIFECYCLE=1`, full 1280×720 dumps were captured at swap 600. The application FBO and game FBO had identical full-file SHA-256:

```text
app.ppm  e9a32d4077d30e83f5d4fd328cf5375e264088ffebd6d7112c2fa36743b421f3
game.ppm e9a32d4077d30e83f5d4fd328cf5375e264088ffebd6d7112c2fa36743b421f3
```

`present-context.ppm` and `window.ppm` matched that same hash. Pixel scan found non-black app content only at y=91..627 before the fix. A separate physical capture of the later menu frame had active y=91..629. Therefore the bars were already present in the game's application render target, not introduced by the presenter/window handoff.

The diagnostic log also showed repeated game requests for viewport/scissor `(0,90,1280,540)` and the bridge rewrite to `(0,0,1280,720)`. That rewrite alone did not remove the bars inside the application FBO.

## Change

- Keep offscreen SDL and bridge dimensions at 1280×720 by default.
- Keep the existing 720p viewport/scissor rewrite.
- Add a presenter source-texture crop, opt-in via `GUACAMELEE_PRESENT_SOURCE_CROP_90`; it activates only for a 1280×720 source and samples v=`90/720..630/720` while drawing to the full destination rectangle.
- Set the verified option on by default in the Guacamelee launcher; `GUACAMELEE_PRESENT_SOURCE_CROP_90=0` disables it for A/B diagnostics.
- No SDL library or audio backend was replaced for this fix.

The crop changes texture coordinates only; it does not alter game FBO contents, guest GL state, audio, or input transport.

## Build and device verification

Built `src/glbridge/server.c` as the AArch64 presenter using `aarch64-linux-gnu-gcc -O2 -s -fno-unwind-tables -fno-asynchronous-unwind-tables -I src/glbridge ... -ldl`. The artifact is an AArch64 ELF with maximum required GLIBC version 2.17.

```text
SHA-256: b6695b322eb157fcdf87b79063e6cefea199a02633fbe7f66946e417a13728b9
```

`python3 tools/test_port.py`, `bash -n portmaster/Guacamelee.sh`, `git diff --check`, and the target cross-build passed. On TSPS, the read-back presenter hash matched the build artifact; the launcher exported crop=1, GL diagnostics were loaded, the source-crop log appeared, FMOD initialized, and the physical framebuffer showed active gameplay. User confirmation: graphics correct and sound working.

Rollback copies are on the device at:

```text
/mnt/SDCARD/Persistent/portmaster/guacamelee-presenter-before-source-crop-20260923/
```

The original presenter hash is `0bf0689d4c2c7f6847a7749533b303ac77a7ca5b29105677125c21274e88ddbd`; original launcher hash is `71ee630fa09e7fc4632cb8d1f116c18fd0a87ac9537b2e16197415a9c6b2c757`.

Raw swap dumps and captures remain in the local, untracked `diagnostics-run/20260923/fbo-probe/` directory and are intentionally not added to the PR.
