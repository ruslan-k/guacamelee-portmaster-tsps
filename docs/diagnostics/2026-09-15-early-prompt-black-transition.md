# Early prompt-to-black transition — 2026-09-15

## Run

Production graphics path with early FBO/pixel/title/op diagnostics:

```text
GUACAMELEE_FBO_TRANSITION_MIN_SWAP=2
GUACAMELEE_FBO_TRANSITION_TRACE=1
GUACAMELEE_PIXEL_PROBE=1
GUACAMELEE_PIXEL_DUMP=1
GUACAMELEE_TITLE_DRAW_PROBE=1
GUACAMELEE_TITLE_STATE_DIAG=1
GUACAMELEE_OP_RING=1
```

All probes were removed after the run.

## Timeline

```text
swap 2:
  FBO1/game/window nonblack = 0

swap 3:
  FBO1/game nonblack = 1952/6144
  window nonblack     = 1952/6144
  viewport            = 0,96,1024,576
  last draw           = FBO1

swaps 4..7:
  FBO1 remains the active visible target
  prompt path continues

swap 8..9:
  FBO1 draw sequence changes to program 28 and program 18
  viewport = 0,0,1024,768

swap 10:
  FBO1/game/window nonblack = 0
  FBO1 hash = 1d00c08067be8383
```

The first sampled black frame is therefore produced by the FBO1 render path
before the later FBO2 title composition becomes the active source. The final
zero-area blit is not the cause of the initial prompt disappearance.

## FBO2 relation

This early run captured the first title draws into FBO2 after the FBO1 prompt
had already disappeared. Existing title-draw and post-clear diagnostics show
that FBO2 can later become nonblack in a separate progression, while FBO1 and
the presented window remain black. Therefore the failure has two stages:

1. the prompt/FBO1 path is overwritten by the program-28/program-18 sequence;
2. later title/FBO2 rendering is not successfully propagated to the visible
   FBO1/window path.

The exact per-draw pixel change inside the program-28/program-18 sequence was
not captured by the current swap-level readback; no production fix is claimed.

## Conclusion

The requested prompt-to-black boundary is narrowed to the FBO1 draw sequence
at swaps 8–10, not to SDL size, input, presenter, or the initial zero-area
compose call. The next useful evidence is a per-draw readback/classifier for
program 28 and program 18, followed by comparison with the native draw
sequence. Broad gl4es/presenter workarounds remain closed.
