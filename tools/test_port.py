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
    launcher_source = (ROOT / "portmaster" / "Guacamelee.sh").read_text()
    assert 'TSPGL_WIDTH' in source and 'TSPGL_HEIGHT' in source
    assert 'eglQuerySurface' in source
    assert "src[total++] = '\\n';" in server_source
    assert '$GPTOKEYB2 "game-bin"' in launcher_source
    assert '"$GPTOKEYB2"' not in launcher_source

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
