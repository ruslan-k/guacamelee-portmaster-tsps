# Black title-scene frame — 2026-09-14

## Scope

After the 1024x768 display fix, the Guacamelee splash is centered and visible. The next frame becomes a stable black/fade image with dark corner gradients. This report records the live-state investigation and one reversible FBO A/B.

## Process and IPC evidence

During the first live observation:

- `game-bin` PID 3736, 10 threads;
- `guacamelee_present` PID 3716, 12 threads;
- three DRM captures over four seconds were byte-identical:
  `6a3b32b749d71a0bb603003ca58d63d00472775ca1f5688f4247cbd2165666f6`;
- the game main thread waited on the Unix bridge socket in `/proc`, but a four-second `strace` showed continuous GL request/reply traffic approximately every 20 ms;
- presenter accepted requests and returned replies.

This was not the original Box86 startup stall and was not an IPC deadlock. The process was closed after read-only observation; MainUI returned.

## Game progress

The game reached the title scene:

```text
Loaded binary scene graph 'Map_Intro_StartScreen.level.bin'
Creating Texture: guac_logo [2048x1024]
Creating Texture: guac_main_tilte_screen [2048x1024]
OnDraw: 3
OnDrawThreaded: 15
```

The semantic log also contains:

```text
PlayerList::OnPostActivate: Unable to spawn P0. No spawn point
```

Other errors include FBO incomplete statuses, one `GL_INVALID_ENUM` at `VideoSys.cpp:1644`, missing optional `patch.1.dat`, missing `settings.ini`/`preloadTextures.ini`, and missing optional costume textures.

## FBO diagnostic trace

A rebuilt presenter with the opt-in `GUACAMELEE_GL_DIAG=1` trace was installed and run. The presenter-created game FBO itself was complete:

```text
tspgl-srv: game FBO 1 color=1 depth=1 1024x768 status=0x8cd5
```

The game subsequently created application FBOs 2–6. The first renderbuffer storage requests were:

```text
op=65 GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width=0, height=0
op=65 GL_RENDERBUFFER, GL_DEPTH_STENCIL,   width=0, height=0
op=65 GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width=0, height=0
```

Their first framebuffer checks returned:

```text
0x8cd7  GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT
0x8cd6  GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT
```

The game then repeatedly binds FBOs 2–6 and receives successful protocol replies, but no `glFramebufferTexture2D` operation was observed in the bounded trace. The application FBOs therefore appear to lack usable color attachments or have zero-sized depth/stencil storage. This is the strongest current hypothesis for the black title frame.

## Reversible A/B

An opt-in presenter variant replaced only `0x0` renderbuffer dimensions with `1024x768`:

```text
GUACAMELEE_RB_ZERO_SIZE=1
```

The override executed three times, for formats `0x8056`, `0x8d48`, and `0x81a5`. Result:

- splash remained centered;
- black title frame remained;
- FBO incomplete errors were reduced but not eliminated;
- `GL_INVALID_ENUM` remained;
- `Unable to spawn P0. No spawn point` remained;
- no production default was changed.

The A/B was rolled back. The device is restored to the diagnostic presenter without the override, and no Guacamelee process is running.

## Current conclusion

Proven:

1. 1024x768 fixes the splash displacement.
2. The presenter-owned game FBO is complete.
3. The game reaches `Map_Intro_StartScreen` and issues ongoing GL request/reply traffic.
4. Application-side FBO setup submits zero-sized renderbuffers and incomplete checks.
5. Merely replacing zero dimensions is insufficient.

Not yet proven:

- whether the zero dimensions originate in the game, gl4es, or a bridge state/query translation;
- whether the missing `glFramebufferTexture2D` path is intentional for depth-only tile FBOs or is a bridge omission;
- whether `No spawn point` is fatal for this title scene or a benign callback error.

## ChatGPT consultation

A self-contained high-reasoning request was prepared at `/tmp/guacamelee-chatgpt-black-title.md` and submitted through `chatgpt-use` relay with request-id `guacamelee-black-title-20260914`. The client returned `submission_unknown`; `resume` returned `no_conversation`. No answer was obtained and the request was not resent.
