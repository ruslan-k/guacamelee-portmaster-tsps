# Guacamelee! Gold Edition for PortMaster on TSPS

This repository tracks an experimental PortMaster port for the GOG Linux build of Guacamelee! Gold Edition.

The TSPS backend keeps the original x86 game and Box86 on the 32-bit side, then routes OpenGL 2.1 through the ARMHF gl4es wrapper and a GLES bridge:

```text
x86 game
  -> Box86 ARMHF
  -> gl4es ARMHF
  -> ARMHF GLES/EGL proxy
  -> /tmp/tsp-glbridge.sock
  -> AArch64 presenter
  -> system Mali GLES
```

The presenter and ARMHF sysroot are copied from the known-good Galaxy on Fire 2 bridge architecture. The GLES client/server source is in `src/glbridge/` and includes the Guacamelee-specific configurable surface dimensions.

## Source and game data

The repository does not include the paid game data. Copy the supplied GOG installer into the installed port directory as:

```text
gog_guacamelee_gold_edition_2.0.0.3.sh
```

On first launch `setup.sh` uses PortMaster's `7zzs` extractor and moves `data/noarch/game` into `gamedata`. It does not execute the installer shell script.

## Build the bridge

Install Zig 0.13 or newer and run:

```sh
ZIG=/path/to/zig tools/build_tsps_bridge.sh
```

The script builds both artifacts:

- `glbridge/libEGL.so.1` and GLES aliases: ARMHF client
- `guacamelee_present`: AArch64 presenter

It also verifies the ELF classes and creates the aliases consumed by gl4es.

## Runtime switches

The launcher selects the bridge automatically on Smart Pro S / SpruceOS. For controlled tests:

```text
GUACAMELEE_TSPS_BRIDGE=1     force the bridge
GUACAMELEE_TSPS_BRIDGE=0     use the original direct GLES path
GUACAMELEE_WIDTH=640         guest surface width, default 640
GUACAMELEE_HEIGHT=480        guest surface height, default 480
GUACAMELEE_PRESENT=letterbox presenter scaling
GUACAMELEE_LIBGL_SHRINK=4    gl4es texture reduction
```

Initial bring-up intentionally uses a 640x480 game surface letterboxed onto the 1280x720 TSPS panel. A clean process exit is not functional proof. Device validation must cover presenter readiness, actual frames, controls, audio, saves, exit, and return to PortMaster.

## Status

WIP. Host-side source and packaging checks are required before each device run. No gameplay claim is made until the real PortMaster menu launch is verified on TSPS.
