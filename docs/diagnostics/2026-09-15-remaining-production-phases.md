# Remaining production phases — 2026-09-15

## Completed in this batch

- Removed production dependence on the historical `libgua_hardext_fix.so` preload. The launcher now defaults `GUACAMELEE_HARDEXT_LIB` to `0`; the source-patched gl4es capability profile is the production path. The preload remains opt-in for diagnostics.
- Added VAO/EBO shadow ownership in the bridge and built it successfully.
- Added source-buffer tracking to bridge attribute metadata.
- Corrected client-array staging size from `count * stride` to `(count - 1) * stride + element_size`.
- Reduced the advertised extension list to extensions retained by the validated bridge/Mali intersection; removed unvalidated framebuffer-fetch, KHR_debug, anisotropic, map-buffer-range, multi-draw, texture-3D, program-binary, fbo-mipmap, stencil8 and unpack-subimage claims.

## Verification

```text
bridge build: success
bash -n portmaster/Guacamelee.sh: success
git diff --check: success
```

The production runtime on TSPS remains restored to the distribution binaries. A device run of the VAO candidate was attempted but stopped in the launcher before game startup because this session lacks `/dev/shm/portmaster`; therefore no visual result is claimed for that candidate.

## Remaining

- Permanent SDL/offscreen normalization is still not integrated into the launcher/source; it remains a temporary x86 shim.
- Separate GLES2 game/presenter contexts are not implemented. The bridge now requests GLES2, but the device still reports Mali GLES 3.2.
- A clean physical integration run after the source changes is blocked until the PortMaster `/dev/shm/portmaster` prerequisite is available.
- The existing diagnostic branch should still be split from production before merge.
