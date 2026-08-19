#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import venv

ROOT = Path(__file__).resolve().parents[1]
VENV = ROOT / ".venv"
LOCK = ROOT / "tools" / "toolchain.lock.json"


def venv_python() -> Path:
    return VENV / ("Scripts/python.exe" if os.name == "nt" else "bin/python")


def godot_binary() -> str | None:
    override = os.environ.get("GODOT_BIN")
    if override:
        return override
    return shutil.which("godot") or shutil.which("godot4")


def verify_godot(expected: str) -> None:
    binary = godot_binary()
    if binary is None:
        raise SystemExit(
            f"Godot {expected} is required. Install the exact baseline and set GODOT_BIN if it is not on PATH."
        )
    result = subprocess.run([binary, "--version"], capture_output=True, text=True, timeout=10, check=True)
    actual = result.stdout.strip()
    if not actual.startswith(expected):
        raise SystemExit(f"Godot version mismatch: expected {expected}, got {actual}")
    print(f"Godot OK: {actual}")


def main() -> int:
    parser = argparse.ArgumentParser(description="Bootstrap pinned local build tools and dependencies")
    parser.add_argument("--no-configure", action="store_true", help="install/check tools without fetching C++ dependencies")
    args = parser.parse_args()

    lock = json.loads(LOCK.read_text(encoding="utf-8"))
    if sys.version_info < (3, 12):
        raise SystemExit(f"Python {lock['python_minimum']}+ is required; got {sys.version.split()[0]}")

    if not VENV.exists():
        print(f"Creating {VENV.relative_to(ROOT)}")
        venv.EnvBuilder(with_pip=True).create(VENV)

    python = venv_python()
    subprocess.run(
        [str(python), "-m", "pip", "install", "--disable-pip-version-check", "-r", str(ROOT / "requirements-dev.txt")],
        cwd=ROOT,
        check=True,
    )

    verify_godot(lock["godot"])

    if not args.no_configure:
        subprocess.run([str(python), str(ROOT / "tools" / "dev.py"), "configure", "--preset", "dev"], cwd=ROOT, check=True)

    print("Bootstrap complete")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
