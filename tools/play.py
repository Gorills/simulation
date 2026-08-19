#!/usr/bin/env python3
from __future__ import annotations

import argparse
from contextlib import AbstractContextManager
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import signal
import subprocess
import time
from typing import IO
import uuid

from godot_runtime import PROJECT, ROOT, expected_extension_library, import_project_metadata, resolve_godot

CACHE = ROOT / ".cache" / "play"
LOCK_PATH = CACHE / "godot.lock"
SUPPORTED_LOCALES = ("ru", "en")


class PlayLock(AbstractContextManager["PlayLock"]):
    def __init__(self, path: Path) -> None:
        self.path = path
        self.handle: IO[bytes] | None = None

    def __enter__(self) -> "PlayLock":
        self.path.parent.mkdir(parents=True, exist_ok=True)
        self.handle = self.path.open("a+b")
        self.handle.seek(0)
        self.handle.write(b"0")
        self.handle.flush()
        try:
            if os.name == "nt":
                import msvcrt

                self.handle.seek(0)
                msvcrt.locking(self.handle.fileno(), msvcrt.LK_NBLCK, 1)
            else:
                import fcntl

                fcntl.flock(self.handle.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
        except OSError as exc:
            self.handle.close()
            self.handle = None
            raise SystemExit("PLAYTEST BUSY: another repository playtest owns the lock") from exc
        return self

    def __exit__(self, exc_type, exc, tb) -> None:  # type: ignore[no-untyped-def]
        if self.handle is None:
            return
        try:
            if os.name == "nt":
                import msvcrt

                self.handle.seek(0)
                msvcrt.locking(self.handle.fileno(), msvcrt.LK_UNLCK, 1)
            else:
                import fcntl

                fcntl.flock(self.handle.fileno(), fcntl.LOCK_UN)
        finally:
            self.handle.close()
            self.handle = None


def terminate_owned_process(process: subprocess.Popen[bytes]) -> None:
    if process.poll() is not None:
        return
    if os.name == "nt":
        process.terminate()
    else:
        os.killpg(process.pid, signal.SIGTERM)
    try:
        process.wait(timeout=3)
    except subprocess.TimeoutExpired:
        if os.name == "nt":
            process.kill()
        else:
            os.killpg(process.pid, signal.SIGKILL)
        process.wait(timeout=3)


def validate_smoke_artifact(path: Path, expected_locale: str) -> dict[str, object]:
    debug_path = path / "debug.json"
    screenshot_path = path / "final.png"
    if not debug_path.is_file() or not screenshot_path.is_file():
        raise SystemExit(f"playtest did not produce required artifacts in {path}")

    evidence = json.loads(debug_path.read_text(encoding="utf-8"))
    if not isinstance(evidence, dict):
        raise SystemExit("debug artifact must be a JSON object")

    bootstrap = evidence.get("bootstrap_projection")
    observed = evidence.get("observed_world_projection")
    spatial = evidence.get("controlled_actor_spatial_projection")
    presentation = evidence.get("presentation")
    localization = evidence.get("localization")
    if not all(isinstance(section, dict) for section in (bootstrap, observed, spatial, presentation, localization)):
        raise SystemExit("debug artifact is missing bootstrap/observed/spatial/presentation/localization sections")

    assert isinstance(bootstrap, dict)
    assert isinstance(observed, dict)
    assert isinstance(spatial, dict)
    assert isinstance(presentation, dict)
    assert isinstance(localization, dict)

    expected_bootstrap = {
        "entity_id": 1,
        "x": 1,
        "y": 0,
        "tick": 0,
        "revision": 2,
        "seed": 1,
        "protocol_version": 4,
    }
    for key, value in expected_bootstrap.items():
        if bootstrap.get(key) != value:
            raise SystemExit(f"unexpected bootstrap projection {key}: expected {value}, got {bootstrap.get(key)}")

    expected_observed = {
        "controlled_actor_id": 1,
        "tick": 0,
        "revision": 2,
        "protocol_version": 4,
    }
    for key, value in expected_observed.items():
        if observed.get(key) != value:
            raise SystemExit(f"unexpected observed-world {key}: expected {value}, got {observed.get(key)}")
    if observed.get("entities") != [{"entity_id": 1}]:
        raise SystemExit(f"unexpected observed entities: {observed.get('entities')}")

    expected_spatial = {
        "entity_id": 1,
        "position_m": [0.0, 0.0, 0.0],
        "velocity_mps": [0.0, 0.0, 0.0],
        "spatial_epoch": 1,
        "tick": 0,
        "revision": 2,
        "protocol_version": 4,
    }
    for key, value in expected_spatial.items():
        if spatial.get(key) != value:
            raise SystemExit(f"unexpected controlled spatial {key}: expected {value}, got {spatial.get(key)}")

    expected_presentation = {
        "controlled_entity_id": 1,
        "last_tick": 0,
        "last_revision": 2,
        "protocol_version": 4,
        "observed_entity_ids": [1],
        "bound_entity_ids": [1],
        "controlled_spatial_initialized": True,
        "controlled_spatial_epoch": 1,
        "controlled_spatial_tick": 0,
        "controlled_spatial_revision": 1,
    }
    for key, value in expected_presentation.items():
        if presentation.get(key) != value:
            raise SystemExit(f"unexpected presentation {key}: expected {value}, got {presentation.get(key)}")

    expected_localized_text = {
        "ru": {
            "hud_title": "Диагностика выполнения",
            "controls_hint": "WASD / левый стик — движение  ·  мышь / правый стик — обзор  ·  Shift / L3 — бег  ·  Esc — освободить курсор",
        },
        "en": {
            "hud_title": "Runtime diagnostics",
            "controls_hint": "WASD / left stick move  ·  mouse / right stick look  ·  Shift / L3 sprint  ·  Esc releases pointer",
        },
    }
    if localization.get("locale") != expected_locale:
        raise SystemExit(
            f"unexpected localization locale: expected {expected_locale}, got {localization.get('locale')}"
        )
    if localization.get("supported_locales") != list(SUPPORTED_LOCALES):
        raise SystemExit(f"unexpected supported locales: {localization.get('supported_locales')}")
    for key, value in expected_localized_text[expected_locale].items():
        if localization.get(key) != value:
            raise SystemExit(
                f"unexpected localized {key}: expected {value!r}, got {localization.get(key)!r}"
            )

    return evidence


def main() -> int:
    parser = argparse.ArgumentParser(description="Run one bounded Godot playtest owned by this repository")
    parser.add_argument("--scenario", default="smoke", choices=("smoke",))
    parser.add_argument("--locale", default="ru", choices=SUPPORTED_LOCALES)
    parser.add_argument("--timeout", type=float, default=30.0)
    args = parser.parse_args()

    CACHE.mkdir(parents=True, exist_ok=True)
    run_id = f"{datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')}-{uuid.uuid4().hex[:8]}"
    artifact_dir = CACHE / run_id
    artifact_dir.mkdir(parents=True)
    stdout_path = artifact_dir / "stdout.log"
    stderr_path = artifact_dir / "stderr.log"
    run_path = artifact_dir / "run.json"

    library = expected_extension_library()
    if not library.is_file():
        raise SystemExit(f"GDExtension library missing: {library}; run tools/dev.py build --preset dev")

    godot = resolve_godot()
    import_project_metadata(godot)
    command = [
        godot,
        "--path",
        str(PROJECT),
        "--",
        "--locale",
        args.locale,
        "--scenario",
        args.scenario,
        "--artifact-dir",
        str(artifact_dir),
    ]
    started = time.monotonic()
    metadata: dict[str, object] = {
        "run_id": run_id,
        "scenario": args.scenario,
        "locale": args.locale,
        "command": command,
        "status": "running",
    }
    run_path.write_text(json.dumps(metadata, indent=2), encoding="utf-8")

    with PlayLock(LOCK_PATH), stdout_path.open("wb") as stdout, stderr_path.open("wb") as stderr:
        popen_kwargs: dict[str, object] = {"cwd": ROOT, "stdout": stdout, "stderr": stderr}
        if os.name == "nt":
            popen_kwargs["creationflags"] = subprocess.CREATE_NEW_PROCESS_GROUP
        else:
            popen_kwargs["start_new_session"] = True
        process = subprocess.Popen(command, **popen_kwargs)  # type: ignore[arg-type]
        try:
            return_code = process.wait(timeout=args.timeout)
        except subprocess.TimeoutExpired:
            terminate_owned_process(process)
            metadata.update({"status": "timeout", "elapsed_seconds": time.monotonic() - started})
            run_path.write_text(json.dumps(metadata, indent=2), encoding="utf-8")
            raise SystemExit(f"PLAYTEST TIMEOUT after {args.timeout:.1f}s; artifacts: {artifact_dir}")
        finally:
            if process.poll() is None:
                terminate_owned_process(process)

    metadata.update({"return_code": return_code, "elapsed_seconds": time.monotonic() - started})
    if return_code != 0:
        metadata["status"] = "failed"
        run_path.write_text(json.dumps(metadata, indent=2), encoding="utf-8")
        raise SystemExit(f"Godot playtest failed with exit code {return_code}; artifacts: {artifact_dir}")

    evidence = validate_smoke_artifact(artifact_dir, args.locale)
    metadata.update({"status": "passed", "evidence": evidence})
    run_path.write_text(json.dumps(metadata, indent=2, ensure_ascii=False), encoding="utf-8")
    print(f"PLAYTEST PASSED ({args.locale}): {artifact_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
