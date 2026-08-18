#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path
import shutil
import subprocess
import sys
from typing import Sequence

ROOT = Path(__file__).resolve().parents[1]
PRESETS = ("native-debug", "native-release")


def run(argv: Sequence[str], *, timeout: int = 300) -> None:
    print("+", " ".join(str(part) for part in argv), flush=True)
    subprocess.run([str(part) for part in argv], cwd=ROOT, check=True, timeout=timeout)


def require_tool(name: str) -> str:
    path = shutil.which(name)
    if path is None:
        raise RuntimeError(f"required local tool is missing: {name}")
    return path


def require_image_converter() -> str:
    for name in ("magick", "convert"):
        path = shutil.which(name)
        if path is not None:
            return path
    raise RuntimeError("required local image converter is missing: magick/convert")


def doctor() -> None:
    print(f"platform={sys.platform}", flush=True)
    for name, version_args in (
        ("git", ("--version",)),
        ("g++", ("--version",)),
        ("cmake", ("--version",)),
        ("ninja", ("--version",)),
        ("ctest", ("--version",)),
        ("python3", ("--version",)),
    ):
        run([require_tool(name), *version_args], timeout=10)

    if sys.platform.startswith("linux"):
        # These are required for the canonical headless graphical verification path.
        require_tool("Xvfb")
        run([require_tool("xwininfo"), "-version"], timeout=10)
        converter = require_image_converter()
        run([converter, "-version"], timeout=10)

    run([sys.executable, "tools/vendor_check.py"], timeout=10)


def configure(preset: str = "native-debug") -> None:
    run([require_tool("cmake"), "--preset", preset], timeout=120)


def build(preset: str = "native-debug") -> None:
    run([require_tool("cmake"), "--build", "--preset", preset], timeout=120)


def test(target: str | None = None) -> None:
    argv = [require_tool("ctest"), "--test-dir", "build/native-debug", "--output-on-failure"]
    if target is not None:
        argv.extend(("-L", target))
    run(argv, timeout=60)


def graphics_check() -> None:
    run([sys.executable, "tools/graphics_check.py"], timeout=30)


def docs_check() -> None:
    run([sys.executable, "tools/docs_check.py"], timeout=30)


def vendor_check() -> None:
    run([sys.executable, "tools/vendor_check.py"], timeout=30)


def check() -> None:
    doctor()
    configure("native-debug")
    build("native-debug")
    test()
    run([sys.executable, "-m", "compileall", "-q", "tools"], timeout=30)
    docs_check()


def verify() -> None:
    # Full foundation acceptance for the Linux agent host: static/toolchain checks,
    # native tests, real headless window/input/capture, then release build.
    check()
    graphics_check()
    configure("native-release")
    build("native-release")


def main() -> int:
    parser = argparse.ArgumentParser(description="Network-free repository development front door")
    subparsers = parser.add_subparsers(dest="command", required=True)
    subparsers.add_parser("doctor")
    configure_parser = subparsers.add_parser("configure")
    configure_parser.add_argument("--preset", choices=PRESETS, default="native-debug")
    build_parser = subparsers.add_parser("build")
    build_parser.add_argument("--preset", choices=PRESETS, default="native-debug")
    test_parser = subparsers.add_parser("test")
    test_parser.add_argument("--target", choices=("foundation", "toolchain", "graphics"))
    subparsers.add_parser("graphics-check")
    subparsers.add_parser("docs-check")
    subparsers.add_parser("vendor-check")
    subparsers.add_parser("check")
    subparsers.add_parser("verify")
    args = parser.parse_args()

    try:
        if args.command == "doctor":
            doctor()
        elif args.command == "configure":
            configure(args.preset)
        elif args.command == "build":
            build(args.preset)
        elif args.command == "test":
            test(args.target)
        elif args.command == "graphics-check":
            graphics_check()
        elif args.command == "docs-check":
            docs_check()
        elif args.command == "vendor-check":
            vendor_check()
        elif args.command == "check":
            check()
        elif args.command == "verify":
            verify()
        else:
            parser.error("unknown command")
        return 0
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired, RuntimeError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
