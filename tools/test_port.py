#!/usr/bin/env python3
"""Offline checks for the Guacamelee TSPS package."""
from __future__ import annotations

import hashlib
import json
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
PORT = ROOT / "portmaster" / "guacamelee"


def run(*args: str) -> str:
    return subprocess.check_output(args, cwd=ROOT, text=True, stderr=subprocess.STDOUT)


def main() -> int:
    metadata = json.loads((PORT / "port.json").read_text())
    assert metadata["version"] == 4
    assert metadata["items"] == ["Guacamelee.sh", "guacamelee"]
    assert metadata["attr"]["arch"] == ["aarch64", "armhf"]
    assert (PORT / "gameinfo.xml").read_text().count("./Guacamelee.sh") == 1

    for script in [
        ROOT / "portmaster" / "Guacamelee.sh",
        PORT / "setup.sh",
        ROOT / "tools" / "build_tsps_bridge.sh",
        ROOT / "tools" / "build_hardext_fix.sh",
        ROOT / "tools" / "build_port.sh",
    ]:
        run("bash", "-n", str(script))

    required = [
        PORT / "box86" / "box86",
        PORT / "box86" / "native" / "libSDL2-2.0.so.0",
        PORT / "gl4es" / "libGL.so.1",
        PORT / "compat" / "libgua_hardext_fix.so",
        PORT / "guacamelee_present",
        PORT / "armhf" / "lib" / "ld-linux-armhf.so.3",
        PORT / "glbridge" / "libEGL.so.1",
        PORT / "guacamelee.ini",
    ]
    for path in required:
        assert path.is_file(), path

    game_names = {"game-bin", "resources.dat", "levels.dat", "music.fsb.linux"}
    tracked_game_files = {p.name for p in (PORT / "gamedata").rglob("*") if p.is_file()}
    assert not game_names.intersection(tracked_game_files)

    source = (ROOT / "src" / "glbridge" / "client.c").read_text()
    xport_source = (ROOT / "src" / "glbridge" / "client_xport.c").read_text()
    server_source = (ROOT / "src" / "glbridge" / "server_gl.c").read_text()
    server_main_source = (ROOT / "src" / "glbridge" / "server.c").read_text()
    frame_source = (ROOT / "src" / "nfsmw_frame.h").read_text()
    launcher_source = (ROOT / "portmaster" / "Guacamelee.sh").read_text()
    hardext_source = (ROOT / "compat" / "hardext_fix.c").read_text()
    assert 'TSPGL_WIDTH' in source and 'TSPGL_HEIGHT' in source
    assert 'eglQuerySurface' in source
    assert "src[total++] = '\\n';" in server_source
    assert 'strip_img_ubo' in server_source
    assert '$GPTOKEYB2 "game-bin"' in launcher_source
    assert '"$GPTOKEYB2"' not in launcher_source

    # The bundled gl4es hardware probe intentionally submits desktop GLSL 120
    # shaders with IMG-specific layout qualifiers. Through the 32->64 proxy we
    # want the conservative GLES2 capability baseline instead of probing the
    # AArch64 presenter as if it were a direct guest driver.
    assert 'LIBGL_NOTEST="${GUACAMELEE_LIBGL_NOTEST:-1}"' in launcher_source
    assert 'LIBGL_ES="${GUACAMELEE_LIBGL_ES:-2}"' in launcher_source
    assert 'LIBGL_GL="${GUACAMELEE_LIBGL_GL:-21}"' in launcher_source
    assert 'gl4es_es=$LIBGL_ES gl=$LIBGL_GL notest=$LIBGL_NOTEST' in launcher_source

    # The requested install root must win over a legacy duplicate left under
    # Data/ports; otherwise a correct Roms/PORTS deployment is never tested.
    roms_candidate = '"${directory:+${directory%/}/guacamelee}"'
    legacy_candidate = '/mnt/SDCARD/Data/ports/guacamelee'
    assert roms_candidate in launcher_source
    assert launcher_source.index(roms_candidate) < launcher_source.index(legacy_candidate)
    assert 'GUACAMELEE_GL_DIAG' in launcher_source
    assert 'GUA-GL shader-final:' in server_source
    assert 'GUA-GL shader-compile FAIL' in server_main_source
    assert 'GUA-GL program-link FAIL' in server_main_source
    assert 'GUA-GL first glLinkProgram success' in server_main_source
    assert 'GUA-GL first glUseProgram' in server_main_source
    assert 'GUA-GL first glDraw' in server_main_source
    assert 'GUA-GL first eglSwapBuffers' in server_main_source
    assert 'GUA-GL op#' in server_main_source
    assert 'GUA-FBO call' in server_main_source
    assert 'GUA-FBO result' in server_main_source
    assert 'GUA-FBO wrap-check' in server_main_source
    assert 'GUA-FBO rb-state' in server_main_source
    assert 'GUA-TEX image#' in server_main_source
    assert 'GUA-TEX copy' in server_main_source
    assert 'GUA-DRAW' in server_main_source
    assert 'GUA-SWAP swap=' in server_main_source
    assert 'GUA-PIX swap=' in server_main_source
    assert 'GUA-KHRDBG' in server_main_source
    assert 'GUA-RING' in server_main_source
    assert 'GUACAMELEE_KHR_DEBUG="${GUACAMELEE_KHR_DEBUG:-0}"' in launcher_source
    assert 'GUACAMELEE_OP_RING="${GUACAMELEE_OP_RING:-0}"' in launcher_source
    assert 'GUACAMELEE_PIXEL_PROBE="${GUACAMELEE_PIXEL_PROBE:-0}"' in launcher_source
    assert 'GUACAMELEE_PIXEL_DUMP="${GUACAMELEE_PIXEL_DUMP:-0}"' in launcher_source
    assert 'GUACAMELEE_FBO_TRANSITION_DIAG="${GUACAMELEE_FBO_TRANSITION_DIAG:-0}"' in launcher_source
    assert 'GUACAMELEE_FBO_FORMAT_MATRIX="${GUACAMELEE_FBO_FORMAT_MATRIX:-0}"' in launcher_source
    assert 'GUACAMELEE_GL_ERROR_TRACE="${GUACAMELEE_GL_ERROR_TRACE:-0}"' in launcher_source
    assert 'GUACAMELEE_GL4ES_PACKED_DS_CAP="${GUACAMELEE_GL4ES_PACKED_DS_CAP:-0}"' in launcher_source
    assert 'GUACAMELEE_LIBGL_AUTOMIPMAP="${GUACAMELEE_LIBGL_AUTOMIPMAP:-0}"' in launcher_source
    assert 'GUACAMELEE_MIPMAP_DIAG="${GUACAMELEE_MIPMAP_DIAG:-0}"' in launcher_source
    assert 'GUACAMELEE_SKIP_BAD_MIPMAP="${GUACAMELEE_SKIP_BAD_MIPMAP:-0}"' in launcher_source
    assert 'GUACAMELEE_TITLE_DRAW_PROBE="${GUACAMELEE_TITLE_DRAW_PROBE:-0}"' in launcher_source
    assert 'GUACAMELEE_TITLE_STATE_DIAG="${GUACAMELEE_TITLE_STATE_DIAG:-0}"' in launcher_source
    assert 'GUACAMELEE_VAO_DIAG="${GUACAMELEE_VAO_DIAG:-0}"' in launcher_source
    assert 'GUACAMELEE_TITLE_OCCLUSION_DIAG="${GUACAMELEE_TITLE_OCCLUSION_DIAG:-0}"' in launcher_source
    assert 'TSPGL_PRESENT_LAST_FBO="${GUACAMELEE_PRESENT_LAST_FBO:-0}"' in launcher_source
    assert 'TSPGL_PRESENT_SET_READ_BUFFER="${GUACAMELEE_PRESENT_SET_READ_BUFFER:-0}"' in launcher_source
    assert 'TSPGL_HOLD_SWAP="${GUACAMELEE_HOLD_SWAP:-0}"' in launcher_source
    assert 'GUACAMELEE_TITLE_WHITE_TEX="${GUACAMELEE_TITLE_WHITE_TEX:-0}"' in launcher_source
    assert 'GUACAMELEE_TITLE_WHITE_COLOR="${GUACAMELEE_TITLE_WHITE_COLOR:-0}"' in launcher_source
    assert 'GUACAMELEE_FBO_LIFECYCLE="${GUACAMELEE_FBO_LIFECYCLE:-0}"' in launcher_source
    assert 'LIBGL_FBOFORCETEX="${GUACAMELEE_LIBGL_FBOFORCETEX:-1}"' in launcher_source
    assert 'GUACAMELEE_REAL_GLERROR="${GUACAMELEE_REAL_GLERROR:-0}"' in launcher_source
    assert 'GUACAMELEE_ZERO_VIEWPORT="${GUACAMELEE_ZERO_VIEWPORT:-0}"' in launcher_source
    assert 'GUACAMELEE_HARDEXT_FIX="${GUACAMELEE_HARDEXT_FIX:-0}"' in launcher_source
    assert 'GUACAMELEE_RB_ZERO_SIZE="${GUACAMELEE_RB_ZERO_SIZE:-0}"' in launcher_source
    assert 'GUACAMELEE_FBO_TEXTURE_FALLBACK="${GUACAMELEE_FBO_TEXTURE_FALLBACK:-0}"' in launcher_source
    assert 'GUACAMELEE_UNIFY_DEPTH_STENCIL="${GUACAMELEE_UNIFY_DEPTH_STENCIL:-0}"' in launcher_source
    assert 'GUACAMELEE_FRONTEND_RB_SIZE_FIX="${GUACAMELEE_FRONTEND_RB_SIZE_FIX:-0}"' in launcher_source
    assert 'GUA-FRONTEND rb-size' in hardext_source
    assert 'TSPGL_RB_ZERO_SIZE' in server_main_source
    assert 'TSPGL_RB_FORMAT_FIX' in server_main_source
    assert 'TSPGL_FBO_TEXTURE_FALLBACK' in server_main_source
    assert 'TSPGL_UNIFY_DEPTH_STENCIL' in server_main_source
    assert 'GUA-FBO texture fallback' in (ROOT / "src" / "glbridge" / "server_gen.c").read_text()
    assert 'GUA-FBO zero-rb override' in server_main_source or 'GUA-FBO zero-rb override' in (ROOT / "src" / "glbridge" / "server_gen.c").read_text()
    assert 'LD_PRELOAD="$HARDEXT_FIX"' in launcher_source
    assert 'env -u LD_PRELOAD' in launcher_source
    assert 'GUACAMELEE_GL4ES_PATH="$GAMEDIR/gl4es/libGL.so.1"' in launcher_source
    assert 'RTLD_NOLOAD' in (ROOT / "compat" / "hardext_fix.c").read_text()
    assert 'GUA-HARDEXT' in (ROOT / "compat" / "hardext_fix.c").read_text()
    assert 'frontend rb-storage' in (ROOT / "compat" / "hardext_fix.c").read_text()
    assert 'frontend fb-rb' in (ROOT / "compat" / "hardext_fix.c").read_text()
    assert 'TSPGL_DEPTH_ONLY_READ_NONE' in server_main_source
    assert 'GUACAMELEE_DEPTH_ONLY_READ_NONE="${GUACAMELEE_DEPTH_ONLY_READ_NONE:-0}"' in launcher_source
    assert 'GUACAMELEE_GL_DIAG="${GUACAMELEE_GL_DIAG:-0}"' in launcher_source
    assert 'GUACAMELEE_WIDTH:-1024' in launcher_source
    assert 'GUACAMELEE_HEIGHT:-768' in launcher_source
    assert '#define NFSMW_FRAME_MAX_H 768' in frame_source
    assert 'int game_w = 1024;' in server_main_source
    assert 'int game_h = 768;' in server_main_source
    assert 'tspgl_dimension("TSPGL_WIDTH", 1024)' in source
    assert 'tspgl_dimension("TSPGL_HEIGHT", 768)' in source
    assert 'st_viewport[2] = 1024' in xport_source
    assert 'st_viewport[3] = 768' in xport_source
    assert launcher_source.index('export GUACAMELEE_WIDTH=') < launcher_source.index('export TSPGL_WIDTH=')
    assert 'GUACAMELEE_XPORT_DIAG="${GUACAMELEE_XPORT_DIAG:-0}"' in launcher_source
    for marker in [
        'GUA-XPORT enter',
        'GUA-XPORT lock-acquired',
        'GUA-XPORT sent',
        'GUA-XPORT reply-hdr',
        'GUA-XPORT end',
        'GUA-XPORT glGetIntegerv begin',
        'GUA-XPORT glGetIntegerv end',
    ]:
        assert marker in xport_source
    assert 'diag_calls < 32' in xport_source
    assert 'BOX86_DYNAREC="${GUACAMELEE_BOX86_DYNAREC:-1}"' in launcher_source
    assert 'BOX86_DYNAREC_BIGBLOCK="${GUACAMELEE_BOX86_DYNAREC_BIGBLOCK:-1}"' in launcher_source
    assert 'BOX86_LOG="${GUACAMELEE_BOX86_LOG:-0}"' in launcher_source
    assert 'BOX86_DLSYM_ERROR="${GUACAMELEE_BOX86_DLSYM_ERROR:-0}"' in launcher_source
    assert 'BOX86_DYNAREC_LOG="${GUACAMELEE_BOX86_DYNAREC_LOG:-0}"' in launcher_source
    assert 'box86_dynarec=$BOX86_DYNAREC' in launcher_source

    bridge_hashes = {
        hashlib.sha256((PORT / "glbridge" / name).read_bytes()).hexdigest()
        for name in [
            "libEGL.so",
            "libEGL.so.1",
            "libGLESv1_CM.so",
            "libGLESv1_CM.so.1",
            "libGLESv2.so",
            "libGLESv2.so.2",
        ]
    }
    assert len(bridge_hashes) == 1

    types = run("file", str(PORT / "guacamelee_present"), str(PORT / "glbridge" / "libEGL.so.1"))
    assert "ELF 64-bit" in types and "ARM aarch64" in types
    assert "ELF 32-bit" in types and "ARM" in types
    print("guacamelee package checks: PASS")
    print(f"bridge aliases: {len(bridge_hashes)} unique hash")
    print(f"game data tracked: no ({len(tracked_game_files)} placeholder file)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
