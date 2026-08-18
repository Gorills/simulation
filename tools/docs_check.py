#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import re
import sys
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parents[1]
DOCS = ROOT / "docs"
INDEX = DOCS / "INDEX.md"
LINK = re.compile(r"(?<!!)\[[^\]]*\]\(([^)]+)\)")
STATUS = re.compile(r"^\*\*Status:\*\*\s+(ACTIVE|TEMPORARY)\s*$", re.MULTILINE)


def markdown_files() -> list[Path]:
    files = [ROOT / "README.md", ROOT / "AGENTS.md"]
    files.extend(sorted(DOCS.rglob("*.md")))
    files.extend(sorted((ROOT / "third_party").rglob("*.md")))
    return [path for path in files if path.is_file()]


def local_target(raw_target: str) -> str | None:
    target = raw_target.strip().split(maxsplit=1)[0].strip("<>")
    if not target or target.startswith(("#", "http://", "https://", "mailto:")):
        return None
    return unquote(target.split("#", 1)[0])


def main() -> int:
    failures: list[str] = []
    index_text = INDEX.read_text(encoding="utf-8") if INDEX.is_file() else ""

    for document in markdown_files():
        text = document.read_text(encoding="utf-8")
        for match in LINK.finditer(text):
            target = local_target(match.group(1))
            if target is None:
                continue
            resolved = (document.parent / target).resolve()
            if not resolved.exists():
                relative_document = document.relative_to(ROOT)
                failures.append(f"{relative_document}: unresolved link {target}")

    for document in sorted(DOCS.rglob("*.md")):
        if document == INDEX:
            continue
        text = document.read_text(encoding="utf-8")
        if STATUS.search(text) is None:
            failures.append(f"{document.relative_to(ROOT)}: canonical docs require ACTIVE/TEMPORARY status")
            continue
        relative_to_docs = document.relative_to(DOCS).as_posix()
        if relative_to_docs not in index_text:
            failures.append(f"docs/INDEX.md: missing canonical document {relative_to_docs}")

    if (ROOT / ".github" / "workflows").exists():
        failures.append("CI workflows exist although this repository explicitly has no CI")

    if failures:
        for failure in failures:
            print(f"DOCS ERROR: {failure}", file=sys.stderr)
        return 1
    print("documentation link/index hygiene check passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
