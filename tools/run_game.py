#!/usr/bin/env python3
from __future__ import annotations

import argparse
import subprocess

from godot_runtime import (
    PROJECT,
    ROOT,
    expected_extension_library,
    import_project_metadata,
    resolve_godot,
)

SUPPORTED_LOCALES = ("ru", "en")


def main() -> int:
    parser = argparse.ArgumentParser(description="Run the configured Godot main scene")
    parser.add_argument("--locale", choices=SUPPORTED_LOCALES)
    args = parser.parse_args()

    library = expected_extension_library()
    if not library.is_file():
        raise SystemExit(f"GDExtension library missing: {library}; build the dev preset first")

    godot = resolve_godot()
    import_project_metadata(godot)
    command = [godot, "--path", str(PROJECT)]
    if args.locale is not None:
        command.extend(["--", "--locale", args.locale])
    print("+", " ".join(command))
    return subprocess.run(command, cwd=ROOT, check=False).returncode


if __name__ == "__main__":
    raise SystemExit(main())
