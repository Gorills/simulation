#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CACHE = ROOT / ".cache" / "play"
ERROR_PREFIXES = ("ERROR:", "SCRIPT ERROR:")


def latest_playtest_directory() -> Path:
    runs = [path.parent for path in CACHE.glob("*/run.json") if path.is_file()]
    if not runs:
        raise SystemExit("no completed playtest run metadata found")
    return max(runs, key=lambda path: (path / "run.json").stat().st_mtime_ns)


def main() -> int:
    run_dir = latest_playtest_directory()
    stderr_path = run_dir / "stderr.log"
    if not stderr_path.is_file():
        raise SystemExit(f"playtest stderr is missing: {stderr_path}")

    unexpected = [
        line
        for line in stderr_path.read_text(encoding="utf-8", errors="replace").splitlines()
        if line.lstrip().startswith(ERROR_PREFIXES)
    ]
    if unexpected:
        print(f"unexpected Godot runtime errors in {stderr_path}:")
        for line in unexpected:
            print(line)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
