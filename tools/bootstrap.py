#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import venv

from godot_runtime import LOCK, expected_godot_version, resolve_godot

ROOT = Path(__file__).resolve().parents[1]
VENV = ROOT / ".venv"


def venv_python() -> Path:
    return VENV / ("Scripts/python.exe" if os.name == "nt" else "bin/python")


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
        [
            str(python),
            "-m",
            "pip",
            "install",
            "--disable-pip-version-check",
            "--no-cache-dir",
            "-r",
            str(ROOT / "requirements-dev.txt"),
        ],
        cwd=ROOT,
        check=True,
    )

    godot = resolve_godot()
    print(f"Godot OK: {expected_godot_version()} at {godot}")

    if not args.no_configure:
        subprocess.run([str(python), str(ROOT / "tools" / "dev.py"), "configure", "--preset", "dev"], cwd=ROOT, check=True)

    print("Bootstrap complete")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
