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
SUPPORTED_LOCALES = ("ru", "en")


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
        command.add_argument(
            "--preset",
            default="dev",
            choices=("native", "sanitize", "dev", "release"),
        )
    play = sub.add_parser("play")
    play.add_argument("--scenario", default="smoke")
    play.add_argument("--locale", default="ru", choices=SUPPORTED_LOCALES)
    run_game = sub.add_parser("run", help="launch the configured main scene without opening the Godot editor")
    run_game.add_argument("--locale", choices=SUPPORTED_LOCALES)

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
        run([sys.executable, str(ROOT / "tools" / "check_localization.py")])
    elif args.command == "play":
        run(
            [
                sys.executable,
                str(ROOT / "tools" / "play.py"),
                "--scenario",
                args.scenario,
                "--locale",
                args.locale,
            ]
        )
        run([sys.executable, str(ROOT / "tools" / "check_playtest_stderr.py")])
    elif args.command == "run":
        argv = [sys.executable, str(ROOT / "tools" / "run_game.py")]
        if args.locale is not None:
            argv.extend(["--locale", args.locale])
        run(argv)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
