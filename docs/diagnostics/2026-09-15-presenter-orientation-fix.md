# Presenter orientation fix — 2026-09-15

The corrected SDL share attribute (`SDL_GL_SHARE_WITH_CURRENT_CONTEXT = 22`) produced a visible presenter frame on TSPS. The user then reported that the image was upside down.

Applied the minimal orientation fix in the presenter quad: inverted only the V texture coordinates. No game rendering, gl4es profile, context, or FBO behavior was changed.

Build/deployment:

```text
python3 tools/test_port.py: PASS
ARM bridge build: PASS
git diff --check: PASS
presenter deployed: PASS
sha256: 35358418570d8eb3816d135564e24156fb786a50bc860c9d8cb2d46dc9a99151
```

Device state after deployment:

```text
game-bin: stopped
guacamelee_present: stopped
gptokeyb2: stopped
MainUI: active
```

The remaining acceptance check is physical: launch from MainUI and confirm the picture is upright, then test input, audio, save/exit and return to MainUI. SSH readback cannot prove physical orientation.
