#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "third_party" / "manifest.json"
HEX40 = re.compile(r"^[0-9a-f]{40}$")
HEX64 = re.compile(r"^[0-9a-f]{64}$")


def main() -> int:
    data = json.loads(MANIFEST.read_text(encoding="utf-8"))
    failures: list[str] = []
    if not isinstance(data, dict) or not data:
        failures.append("manifest must contain at least one dependency")

    for dependency_name, dependency in data.items():
        if not isinstance(dependency, dict):
            failures.append(f"{dependency_name}: dependency entry must be an object")
            continue
        for key in ("upstreamRepository", "upstreamCommit", "license", "files"):
            if key not in dependency:
                failures.append(f"{dependency_name}: missing manifest field {key}")
        commit = dependency.get("upstreamCommit")
        if not isinstance(commit, str) or HEX40.fullmatch(commit) is None:
            failures.append(f"{dependency_name}: upstreamCommit must be a full 40-character Git SHA")
        source_blob = dependency.get("upstreamSourceBlobSha")
        if source_blob is not None and (not isinstance(source_blob, str) or HEX40.fullmatch(source_blob) is None):
            failures.append(f"{dependency_name}: upstreamSourceBlobSha must be a full 40-character Git blob SHA")

        files = dependency.get("files")
        if not isinstance(files, dict):
            failures.append(f"{dependency_name}: manifest has no files map")
            continue
        for relative_path, expected_hash in files.items():
            if not isinstance(relative_path, str) or not isinstance(expected_hash, str) or HEX64.fullmatch(expected_hash) is None:
                failures.append(f"{dependency_name}: invalid file/hash entry {relative_path!r}")
                continue
            path = ROOT / relative_path
            if not path.is_file():
                failures.append(f"{dependency_name}: missing {relative_path}")
                continue
            actual_hash = hashlib.sha256(path.read_bytes()).hexdigest()
            if actual_hash != expected_hash:
                failures.append(
                    f"{dependency_name}: integrity mismatch for {relative_path}: "
                    f"expected {expected_hash}, got {actual_hash}"
                )

    if failures:
        for failure in failures:
            print(f"VENDOR ERROR: {failure}", file=sys.stderr)
        return 1
    print("vendor integrity check passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
