# Early display-global A/B and keyboard ingress — 2026-09-15

## Early display-global A/B

An i386 SDL preload wrote the application display globals immediately after
`SDL_GetWindowSize` / `SDL_GL_GetDrawableSize` returned `1024x768`, before the
normal render-target path. Two representations were tested separately:

```text
float mode: globals = 0x44800000,0x44400000 (1024.0f,768.0f)
int mode:   globals = 0x00000400,0x00000300 (1024,768)
```

Both runs still showed the same progression:

```text
swap 3 prompt frame: nonblack=2080/6144
swap 10/30:          nonblack=0/6144
```

No validated title improvement occurred. The early SDL-global injection was
rolled back. This means either the game overwrites/skips the values later, the
fields are not the actual source of the title render dimensions, or the source
FBO/title-state problem remains independently causal. The late metadata A/B
must not be promoted.

## Keyboard ingress probe

An i386 `SDL_PollEvent` preload was run for 20 seconds without physical input.
It captured:

```text
SDL key events: 0
```

This is only a no-input observation, not proof that gptokeyb2 keyboard ingress
is broken. A physical A/Start press is still required for a conclusive input
phase.

## Restoration

Temporary probes were removed. Production `libGL.so.1` remained:

```text
6b0c60b1f942a9a408d83bc6fdc46a44cbd7195aed482e7fe1cb43420f253253
```

MainUI was active and no game/helper processes remained.
