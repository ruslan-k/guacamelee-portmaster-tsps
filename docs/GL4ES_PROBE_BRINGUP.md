# Guacamelee TSPS: gl4es probe / shader bring-up

## What the current failure actually is

The first suspicious shader is not a Guacamelee shader. It is gl4es' own hardware-capability probe.

The bundled `gl4es/libGL.so.1` contains the same probe strings as upstream gl4es:

```glsl
#version 120
#extension require GL_IMG_uniform_buffer_object
layout(location = 0) in vec4 vecPos;
layout(location = 0) uniform mat4 matMVP;
```

Upstream gl4es sends that shader from `testGLSL()` while detecting whether the backend can accept desktop GLSL 1.20 / IMG-specific layout syntax. It also probes `#version 300 es` and `#version 310 es`.

That probe makes sense when gl4es talks directly to a vendor GLES implementation. Our topology is different:

```text
Guacamelee i386
  -> Box86 ARMHF
  -> gl4es ARMHF (desktop GL 2.1 -> GLES2)
  -> ARMHF tsp-glbridge proxy
  -> AArch64 presenter
  -> Mali-G57 GLES 3.2
```

The guest-side contract must stay conservative GLES2. The presenter owning a GLES 3.2 context must not make gl4es believe that the ARMHF proxy is a native desktop-GLSL/IMG driver.

The previous bridge-side workaround stripped `GL_IMG_uniform_buffer_object` but left the `#version 120` + `layout(...)` combination. That is still invalid for the GLES compiler and, more importantly, it is treating a capability probe as if it were an application shader.

## Fix in this branch

The launcher now defaults to:

```bash
LIBGL_ES=2
LIBGL_GL=21
LIBGL_NOTEST=1
```

`LIBGL_NOTEST` is the official gl4es switch for skipping its initial PBuffer hardware tests. In this mode gl4es uses its conservative GLES2 baseline instead of compiling the desktop-GLSL probe shaders.

The setting is rollbackable:

```bash
GUACAMELEE_LIBGL_NOTEST=0 ./Guacamelee.sh
```

Do not use `0` as the default until the bridge has a filtered capability profile.

## Required physical test #1

Deploy this branch without making any other graphics changes.

Expected launcher marker:

```text
gl4es_es=2 gl=21 notest=1 ...
```

If gl4es logging is visible, `Hardware test disabled, nothing activated...` is also expected.

The old startup probe should disappear. In particular, before real game rendering starts there should no longer be a bridge dump containing all of these together:

```text
#version 120
GL_IMG_uniform_buffer_object
layout(location = 0)
```

Keep the existing successful milestones:

```text
presenter starts
Mali-G57 context created
FBO 640x480 created
ARMHF client connects
Box86 initializes
native wrapped SDL2 loads
misc.dat opens
levels.dat opens
resources.dat opens
preinit.ini parses
```

The next success criterion is not merely 'process stays alive'. We need one of:

1. the first real application shader compiles and links;
2. the first draw call reaches the server;
3. the first `eglSwapBuffers` / present occurs;
4. a visible frame appears.

## If the game still stops after `preinit.ini`

Do not immediately add another global shader rewrite. First locate the exact boundary.

### Step A: prove whether a real shader fails

Add env-gated diagnostics around the existing server special handlers:

- `OP_glGetShaderiv` when `pname == GL_COMPILE_STATUS (0x8B81)`;
- `OP_glGetProgramiv` when `pname == GL_LINK_STATUS (0x8B82)`.

When status is false, log:

```text
shader/program id
status pname
GL error
shader info log / program info log
```

For shaders, retain or dump the final source actually passed to the AArch64 driver, not only the pre-rewrite input.

Suggested marker format:

```text
GUA-GL shader-compile FAIL id=<n> bytes=<n>
GUA-GL shader-log: <driver text>
GUA-GL shader-final:\n<source>
GUA-GL program-link FAIL id=<n>
GUA-GL program-log: <driver text>
```

Limit dumps to the first few failures so logs remain usable.

### Step B: distinguish probe shader from application shader

A shader containing the exact `vecPos` / `matMVP` test above is gl4es hardware detection and should not be 'fixed' into a successful shader.

With `LIBGL_NOTEST=1`, seeing that probe means one of these is wrong:

- the variable did not reach gl4es;
- a different `libGL.so.1` is loaded;
- the bundled gl4es ignores `LIBGL_NOTEST` (unlikely; the binary contains the option string).

In that case enable Box86 library logging and confirm the exact ARMHF `gl4es/libGL.so.1` path before touching the bridge.

### Step C: if an actual shader still contains `#version 120`

Do not blindly convert every desktop GLSL 1.20 shader to `#version 300 es` in the bridge.

Desktop-GLSL -> GLES conversion belongs to gl4es. A real gl4es output reaching the bridge as raw desktop GLSL means gl4es selected the wrong backend capability path.

Check, in this order:

1. `LIBGL_NOTEST=1` is logged by the launcher;
2. `LIBGL_ES=2` is logged;
3. `LIBGL_GLES` points to the ARMHF proxy `libGLESv2.so.2`;
4. `LIBGL_EGL` points to the ARMHF proxy `libEGL.so.1`;
5. Box86 actually loads the bundled gl4es library;
6. no inherited `LIBGL_*` variables override the intended values.

Only after that should shader translation be changed.

## If conservative no-test mode gets to the first frame

Keep `LIBGL_NOTEST=1` for the initial playable port.

Later, if Guacamelee needs an extension missing from the conservative defaults, do not re-enable unrestricted probing. Implement a bridge capability profile instead, for example:

```text
TSPGL_PROFILE=gles2-safe
```

That profile should expose only GLES extensions for which both of these are true:

1. the AArch64 presenter supports the feature;
2. the ARMHF proxy implements every function/data-path needed by gl4es for it.

The current bridge forwards the real Mali `glGetString(GL_EXTENSIONS)`. That can over-advertise features because the presenter is richer than the proxy. `LIBGL_NOTEST=1` avoids this during bring-up.

## Recommended order for the agent

1. Build/package this PR as-is.
2. Run one physical TSPS test with default `GUACAMELEE_LIBGL_NOTEST=1`.
3. Preserve the complete log.
4. Confirm the old GLSL 120 hardware probe is gone.
5. If a frame appears, stop shader work and move to input/audio/performance validation.
6. If it still stalls, add only compile/link/final-source diagnostics described above.
7. Fix the first proven application-shader error, one variable at a time.
8. Do not combine shader changes with SDL, input, audio, resolution, or performance tuning in the same physical test.

## Rollback / A-B switches

Default safe path:

```bash
GUACAMELEE_LIBGL_NOTEST=1
```

Re-enable old gl4es probe behavior for a controlled comparison only:

```bash
GUACAMELEE_LIBGL_NOTEST=0
```

Backend version knobs are also explicit now:

```bash
GUACAMELEE_LIBGL_ES=2
GUACAMELEE_LIBGL_GL=21
```

Do not move to ES3 merely because the presenter reports GLES 3.2. gl4es is the desktop-GL compatibility layer and the current guest/proxy API is GLES2-oriented.
