#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
VENV_BIN = ROOT / ".venv" / ("Scripts" if os.name == "nt" else "bin")


def tool(name: str) -> str:
    local = VENV_BIN / (f"{name}.exe" if os.name == "nt" else name)
    if local.exists():
        return str(local)
    found = shutil.which(name)
    if found:
        return found
    raise SystemExit(f"missing {name}; run: {sys.executable} tools/bootstrap.py")


def run(argv: list[str]) -> None:
    env = os.environ.copy()
    env["PATH"] = os.pathsep.join([str(VENV_BIN), env.get("PATH", "")])
    print("+", " ".join(argv))
    subprocess.run(argv, cwd=ROOT, env=env, check=True)


def main() -> int:
    parser = argparse.ArgumentParser(description="World Simulation local development front door")
    sub = parser.add_subparsers(dest="command", required=True)
    for name in ("configure", "build", "test", "check"):
        command = sub.add_parser(name)
        command.add_argument("--preset", default="dev", choices=("native", "dev", "release"))
    play = sub.add_parser("play")
    play.add_argument("--scenario", default="smoke")

    args = parser.parse_args()
    if args.command == "configure":
        run([tool("cmake"), "--preset", args.preset])
    elif args.command == "build":
        run([tool("cmake"), "--build", "--preset", args.preset])
    elif args.command == "test":
        run([tool("ctest"), "--preset", args.preset])
    elif args.command == "check":
        run([tool("cmake"), "--preset", args.preset])
        run([tool("cmake"), "--build", "--preset", args.preset])
        run([tool("ctest"), "--preset", args.preset])
    elif args.command == "play":
        run([sys.executable, str(ROOT / "tools" / "play.py"), "--scenario", args.scenario])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
