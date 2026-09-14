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
        ROOT / "tools" / "build_port.sh",
    ]:
        run("bash", "-n", str(script))

    required = [
        PORT / "box86" / "box86",
        PORT / "box86" / "native" / "libSDL2-2.0.so.0",
        PORT / "gl4es" / "libGL.so.1",
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
    server_source = (ROOT / "src" / "glbridge" / "server_gl.c").read_text()
    server_main_source = (ROOT / "src" / "glbridge" / "server.c").read_text()
    launcher_source = (ROOT / "portmaster" / "Guacamelee.sh").read_text()
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
