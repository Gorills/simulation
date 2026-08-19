#!/usr/bin/env python3
from __future__ import annotations

import subprocess

from godot_runtime import (
    PROJECT,
    ROOT,
    expected_extension_library,
    import_project_metadata,
    resolve_godot,
)


def main() -> int:
    library = expected_extension_library()
    if not library.is_file():
        raise SystemExit(f"GDExtension library missing: {library}; build the dev preset first")

    godot = resolve_godot()
    import_project_metadata(godot)
    command = [godot, "--path", str(PROJECT)]
    print("+", " ".join(command))
    return subprocess.run(command, cwd=ROOT, check=False).returncode


if __name__ == "__main__":
    raise SystemExit(main())
