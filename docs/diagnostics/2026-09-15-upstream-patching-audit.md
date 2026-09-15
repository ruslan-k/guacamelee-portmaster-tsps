# Upstream PortMaster patching audit and fresh game-bin A/B — 2026-09-15

## Upstream comparison

The upstream `PortsMaster/PortMaster-New` Guacamelee launcher uses the generic
PortMaster patcher:

```text
PATCHER_FILE=$GAMEDIR/tools/extractscript
PATCHER_GAME=Guacamelee
if [ ! -f patchlog.txt ]; then source patcher.txt; fi
```

Its `extractscript` extracts the user-provided GOG/Humble installer into
`gamedata`. The local TSPS launcher intentionally uses `setup.sh` instead of
that generic patcher because the project needs the TSPS bridge, presenter, and
custom runtime environment.

The TSPS `setup.sh` contains the Guacamelee-specific compatibility patch:

```text
file offset 0x858da3:
  e8 08 b0 7a ff  ->  90 90 90 90 90
```

This is the five-byte CPU-affinity patch.

## Hash audit

The clean game-bin extracted from the user's GOG installer is:

```text
cfab463cab9f734588bae0a9c102be89699c747ed0e51235e384eb02cff08d6b
```

The TSPS production game-bin is:

```text
5aa2a2cc89d79912a4036ce5aca344fa6cc6f8767e346b8d271c3b76300aa063
```

All checked non-executable game data and libraries match the clean installer:
`levels.dat`, `resources.dat`, `shaders.dat.ogl`, all FSB/audio files, SDL2,
and both FMOD libraries.

Only `game-bin` differs, and the difference is exactly the five-byte patch.

## Fresh unpatched A/B

The clean unpatched `game-bin` was deployed temporarily with the proven SDL
mode shim active. It did not reach archive parsing, SDL size, or title render.
The Box86/glbridge client dropped immediately after initial `glGetIntegerv`
traffic:

```text
tspgl: 32-bit GLES client loaded
tspgl: shared sock=3
tspgl-srv: client fd=21 drop
exit_code=143
```

The patched production binary reaches normal archive parsing and the prompt.
Therefore the five-byte patch is required for startup; replacing it with the
clean binary is invalid.

## Restoration

The patched production binary was restored and verified. The clean original
was retained as the explicit setup marker:

```text
gamedata/game-bin                  5aa2a2cc...
gamedata/game-bin.affinity-original cfab463c...
```

No upstream launcher overwrite was performed because it would remove the
TSPS bridge launcher path and reintroduce a known-invalid unpatched extraction
state. No production graphics change was made.
