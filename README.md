## Notes

- This is an experimental PortMaster package for the GOG Linux build of *Guacamelee! Gold Edition*.
- The game and its licensed GOG data are not included. You must own the game and supply the installer described below.
- The package has been confirmed working on the user's TrimUI Smart Pro S running SpruceOS. This does not imply testing on other devices or CFWs.
- Thanks to Cebion and kotzebuedog for the earlier porting work credited in the package metadata.

## Runtime on the tested TSPS device

The game is the 32-bit x86 GOG build. Box86 runs it with an ARMHF gl4es and SDL stack; the ARMHF GLES/EGL bridge forwards rendering to the AArch64 presenter and the device's Mali GLES driver:

```text
x86 game -> Box86 -> ARMHF SDL2/gl4es -> ARMHF GLES/EGL bridge
         -> /tmp/tsp-glbridge.sock -> AArch64 presenter -> Mali GLES
```

The working TSPS setup uses a 1280x720 guest/bridge surface. The global `1280x540` viewport/scissor rewrite is disabled by default because the device-confirmed working state requires it off. The presenter source crop remains enabled by default. Diagnostic probes and pixel dumps are off by default.

The runtime binaries in `portmaster/guacamelee/` were synchronized from the user-confirmed working device state on 2026-09-23. Their SHA-256 values are:

| File | SHA-256 |
| --- | --- |
| `guacamelee_present` | `51d89c40fb385fe0a82ece69c0cf00066dea3fbda84352838a683b4a953cce83` |
| `gl4es/libGL.so.1` | `a6f624ab5f160b4ea58e0c8c3722b8de0c19ad076e234df9c6e16c194570edaf` |
| `box86/native/libSDL2-2.0.so.0` | `a33520a8519d1e97e69467a3cbc41c407c3239c1c605ac23bea61a03d08afdb4` |
| `compat/libgua_sdl_mode_input_fix.so` | `a410db8332df48feea63804eb924898cb39e922e061cce5f482d9f45c810c922` |

A successful build of a replacement is not proof that it reproduces this runtime. Validate any changed binary on the device before replacing the known-working artifact.

## Install and provide game data

1. Install the PortMaster package `guacamelee.zip`.
2. Copy the legally owned GOG installer, named exactly `gog_guacamelee_gold_edition_2.0.0.3.sh`, into the installed `guacamelee` data directory.
3. Launch Guacamelee from PortMaster. On first launch, `setup.sh` uses PortMaster's `7zzs` to extract the installer as an archive; it does not execute the installer script. It places the game files in `gamedata/` and applies the TSPS CPU-affinity compatibility patch to `gamedata/game-bin`.

The patch changes five bytes at offset `0x858da3`, from `e808b07aff` to NOPs. `setup.sh` retains an `.affinity-original` copy when it applies the patch. Do not remove `gamedata/`, `conf/`, or the saves under `conf/` when cleaning diagnostics.

## Controls

The bundled `guacamelee.ini` maps:

- D-pad and left stick: movement
- A/B/X/Y: `z` / `x` / `c` / `v`
- L1/R1/L2/R2: `q` / `e` / `a` / `s`
- Start: Enter; Back: Escape
- In menus, A confirms and B backs out

## Build and validation

- `tools/build_tsps_bridge.sh` builds the ARMHF GLES/EGL bridge and AArch64 presenter from `src/glbridge/` using Zig.
- `tools/build_gl4es_tsps_container.sh` builds the TSPS gl4es artifact.
- `tools/build_sdl_mode_input_fix.sh` builds the combined 32-bit SDL mode/input compatibility shim.
- `tools/test_port.py` checks package structure and launcher assumptions. `bash -n portmaster/Guacamelee.sh` and `sh -n portmaster/guacamelee/setup.sh` check shell syntax.

These checks do not replace a real device test. For graphics or input changes, validate a normal PortMaster launch on TSPS, the visible gameplay/HUD, controls, audio, save/load, and clean exit. Keep diagnostic environment variables opt-in; do not leave pixel-dump or verbose GL diagnostics enabled for normal play.
