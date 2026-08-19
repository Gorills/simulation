#!/usr/bin/env python3
from __future__ import annotations

import ast
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
CATALOGS = {
    "ru": ROOT / "godot" / "localization" / "ru.po",
    "en": ROOT / "godot" / "localization" / "en.po",
}
KEY_RE = re.compile(r"^UI_[A-Z0-9_]+$")
SCENE_KEY_RE = re.compile(r'^\s*(?:text|placeholder_text|tooltip_text) = "(UI_[A-Z0-9_]+)"\s*$', re.MULTILINE)
SCRIPT_KEY_RE = re.compile(r'tr\(\s*&?"(UI_[A-Z0-9_]+)"')


def po_literal(source: str) -> str:
    value = ast.literal_eval(source)
    if not isinstance(value, str):
        raise ValueError(f"expected PO string literal, got {source!r}")
    return value


def parse_catalog(path: Path) -> set[str]:
    keys: set[str] = set()
    for block in path.read_text(encoding="utf-8").split("\n\n"):
        lines = [line.strip() for line in block.splitlines() if line.strip() and not line.startswith("#")]
        msgid_lines = [line for line in lines if line.startswith("msgid ")]
        if not msgid_lines:
            continue
        if len(msgid_lines) != 1:
            raise SystemExit(f"{path}: malformed entry with multiple msgid lines")

        msgid = po_literal(msgid_lines[0][len("msgid ") :])
        if not msgid:
            continue
        if not KEY_RE.fullmatch(msgid):
            raise SystemExit(f"{path}: localization key must be semantic UI_* identifier: {msgid}")
        if msgid in keys:
            raise SystemExit(f"{path}: duplicate localization key: {msgid}")

        translations = []
        for line in lines:
            if line.startswith("msgstr "):
                translations.append(po_literal(line[len("msgstr ") :]))
            elif line.startswith("msgstr["):
                translations.append(po_literal(line.split(" ", 1)[1]))
        if not translations or any(not value for value in translations):
            raise SystemExit(f"{path}: missing translation for {msgid}")
        keys.add(msgid)
    return keys


def referenced_keys() -> set[str]:
    keys: set[str] = set()
    for path in (ROOT / "godot" / "scenes").rglob("*.tscn"):
        keys.update(SCENE_KEY_RE.findall(path.read_text(encoding="utf-8")))
    for path in (ROOT / "godot" / "scripts").rglob("*.gd"):
        keys.update(SCRIPT_KEY_RE.findall(path.read_text(encoding="utf-8")))
    return keys


def main() -> int:
    catalog_keys = {locale: parse_catalog(path) for locale, path in CATALOGS.items()}
    baseline_locale = next(iter(CATALOGS))
    baseline = catalog_keys[baseline_locale]
    for locale, keys in catalog_keys.items():
        if keys != baseline:
            missing = sorted(baseline - keys)
            extra = sorted(keys - baseline)
            raise SystemExit(
                f"localization catalog mismatch for {locale}: missing={missing}, extra={extra}"
            )

    referenced = referenced_keys()
    missing_references = sorted(referenced - baseline)
    if missing_references:
        raise SystemExit(f"localization keys referenced by runtime UI are missing: {missing_references}")

    stale = sorted(baseline - referenced)
    if stale:
        raise SystemExit(f"localization catalog contains unreferenced keys: {stale}")

    print(
        "localization catalogs: PASS "
        f"({len(baseline)} keys, locales={','.join(CATALOGS)})"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
