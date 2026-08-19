from __future__ import annotations

import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / "godot"
LOCK = ROOT / "tools" / "toolchain.lock.json"


def expected_godot_version() -> str:
    lock = json.loads(LOCK.read_text(encoding="utf-8"))
    value = lock.get("godot")
    if not isinstance(value, str) or not value:
        raise SystemExit("tools/toolchain.lock.json is missing a valid Godot version")
    return value


def resolve_godot() -> str:
    expected = expected_godot_version()
    override = os.environ.get("GODOT_BIN")
    candidate = override or shutil.which("godot") or shutil.which("godot4")
    if not candidate:
        raise SystemExit(f"Godot {expected} not found; install it or set GODOT_BIN")

    result = subprocess.run(
        [candidate, "--version"],
        capture_output=True,
        text=True,
        timeout=10,
        check=True,
    )
    actual = result.stdout.strip()
    if not actual.startswith(expected):
        raise SystemExit(f"Godot version mismatch: expected {expected}, got {actual}")
    return candidate


def import_project_metadata(godot: str) -> None:
    command = [godot, "--headless", "--path", str(PROJECT), "--import"]
    print("+", " ".join(command))
    try:
        subprocess.run(command, cwd=ROOT, check=True, timeout=120)
    except subprocess.TimeoutExpired as exc:
        raise SystemExit("Godot project import timed out after 120 seconds") from exc


def expected_extension_library() -> Path:
    if sys.platform.startswith("linux"):
        return PROJECT / "bin" / "libworld_sim.template_debug.so"
    if sys.platform == "darwin":
        return PROJECT / "bin" / "libworld_sim.template_debug.dylib"
    if os.name == "nt":
        return PROJECT / "bin" / "world_sim.template_debug.dll"
    raise SystemExit(f"unsupported local Godot platform: {sys.platform}")
