#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path
import shutil
import subprocess
import sys
from typing import Sequence

ROOT = Path(__file__).resolve().parents[1]


def run(argv: Sequence[str], *, timeout: int = 300) -> None:
    print("+", " ".join(str(part) for part in argv), flush=True)
    subprocess.run(
        [str(part) for part in argv],
        cwd=ROOT,
        check=True,
        timeout=timeout,
    )


def require_tool(name: str) -> str:
    path = shutil.which(name)
    if path is None:
        raise RuntimeError(f"Required local tool is missing: {name}")
    return path


def doctor() -> None:
    for name, version_args in (
        ("g++", ("--version",)),
        ("cmake", ("--version",)),
        ("ninja", ("--version",)),
        ("ctest", ("--version",)),
        ("python3", ("--version",)),
    ):
        path = require_tool(name)
        run([path, *version_args], timeout=10)


def configure() -> None:
    run([require_tool("cmake"), "--preset", "native-debug"], timeout=120)


def build() -> None:
    run([require_tool("cmake"), "--build", "--preset", "native-debug"], timeout=120)


def test(target: str | None = None) -> None:
    argv = [
        require_tool("ctest"),
        "--test-dir",
        "build/native-debug",
        "--output-on-failure",
    ]
    if target is not None:
        argv.extend(("-L", target))
    run(argv, timeout=60)


def check() -> None:
    doctor()
    configure()
    build()
    test()
    run([sys.executable, "-m", "compileall", "-q", "tools"], timeout=30)


def main() -> int:
    parser = argparse.ArgumentParser(description="Network-free local development front door")
    subparsers = parser.add_subparsers(dest="command", required=True)
    subparsers.add_parser("doctor")
    subparsers.add_parser("configure")
    subparsers.add_parser("build")
    test_parser = subparsers.add_parser("test")
    test_parser.add_argument("--target", choices=("sim", "protocol", "determinism"))
    subparsers.add_parser("check")
    args = parser.parse_args()

    try:
        if args.command == "doctor":
            doctor()
        elif args.command == "configure":
            configure()
        elif args.command == "build":
            build()
        elif args.command == "test":
            test(args.target)
        elif args.command == "check":
            check()
        else:
            parser.error("Unknown command")
        return 0
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired, RuntimeError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
