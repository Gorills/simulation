#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
PROTECTED = (ROOT / "src" / "sim", ROOT / "src" / "protocol")
FORBIDDEN_INCLUDES = ("#include <godot_cpp/", '#include "godot_cpp/', "#include <godot/")


def main() -> int:
    failures: list[str] = []

    for directory in PROTECTED:
        for path in sorted(directory.rglob("*")):
            if path.suffix not in {".cpp", ".cc", ".cxx", ".h", ".hpp"}:
                continue
            text = path.read_text(encoding="utf-8")
            for marker in FORBIDDEN_INCLUDES:
                if marker in text:
                    failures.append(f"{path.relative_to(ROOT)} contains forbidden dependency marker {marker!r}")

    if failures:
        print("ARCHITECTURE CHECK FAILED", file=sys.stderr)
        for failure in failures:
            print(f"- {failure}", file=sys.stderr)
        return 1

    print("ARCHITECTURE CHECK OK: Simulation Core and protocol are Godot-free")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
