#!/usr/bin/env python3
from __future__ import annotations

import argparse
from contextlib import AbstractContextManager
from datetime import datetime, timezone
import json
import math
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
SMOKE_AUDIO_DRIVER = "Dummy"


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


def require_close_vector(actual: object, expected: list[float], label: str) -> None:
    if not isinstance(actual, list) or len(actual) != len(expected):
        raise SystemExit(f"unexpected {label}: expected {expected}, got {actual}")
    for index, expected_value in enumerate(expected):
        value = actual[index]
        if not isinstance(value, (int, float)) or not math.isclose(
            float(value), expected_value, rel_tol=0.0, abs_tol=1e-5
        ):
            raise SystemExit(
                f"unexpected {label}[{index}]: expected {expected_value}, got {value}"
            )


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
    movement_stream = evidence.get("movement_stream")
    presentation = evidence.get("presentation")
    localization = evidence.get("localization")
    if not all(
        isinstance(section, dict)
        for section in (bootstrap, observed, spatial, movement_stream, presentation, localization)
    ):
        raise SystemExit(
            "debug artifact is missing bootstrap/observed/spatial/movement/presentation/localization sections"
        )

    assert isinstance(bootstrap, dict)
    assert isinstance(observed, dict)
    assert isinstance(spatial, dict)
    assert isinstance(movement_stream, dict)
    assert isinstance(presentation, dict)
    assert isinstance(localization, dict)

    expected_bootstrap = {
        "entity_id": 1,
        "x": 1,
        "y": 0,
        "tick": 0,
        "revision": 3,
        "seed": 1,
        "protocol_version": 6,
    }
    for key, value in expected_bootstrap.items():
        if bootstrap.get(key) != value:
            raise SystemExit(f"unexpected bootstrap projection {key}: expected {value}, got {bootstrap.get(key)}")

    expected_observed = {
        "controlled_actor_id": 1,
        "tick": 1,
        "revision": 4,
        "protocol_version": 6,
    }
    for key, value in expected_observed.items():
        if observed.get(key) != value:
            raise SystemExit(f"unexpected observed-world {key}: expected {value}, got {observed.get(key)}")
    if observed.get("entities") != [{"entity_id": 1}, {"entity_id": 2}]:
        raise SystemExit(f"unexpected observed entities: {observed.get('entities')}")

    expected_spatial_scalars = {
        "entity_id": 1,
        "spatial_epoch": 1,
        "tick": 1,
        "revision": 4,
        "protocol_version": 6,
    }
    for key, value in expected_spatial_scalars.items():
        if spatial.get(key) != value:
            raise SystemExit(f"unexpected controlled spatial {key}: expected {value}, got {spatial.get(key)}")
    require_close_vector(spatial.get("position_m"), [0.001, 0.0, 0.0], "controlled spatial position")
    require_close_vector(spatial.get("velocity_mps"), [0.1, 0.0, 0.0], "controlled spatial velocity")

    if movement_stream.get("duplicate_batch_rejected") is not True:
        raise SystemExit("WorldPresentation did not prove duplicate movement-batch rejection")
    batch = movement_stream.get("batch")
    if not isinstance(batch, dict):
        raise SystemExit("movement stream is missing its authoritative batch")
    expected_batch_header = {"tick": 1, "revision": 4, "protocol_version": 6}
    for key, value in expected_batch_header.items():
        if batch.get(key) != value:
            raise SystemExit(f"unexpected movement batch {key}: expected {value}, got {batch.get(key)}")
    samples = batch.get("samples")
    if (
        not isinstance(samples, list)
        or len(samples) != 2
        or not isinstance(samples[0], dict)
        or not isinstance(samples[1], dict)
    ):
        raise SystemExit(f"unexpected movement samples: {samples}")

    controlled_sample = samples[0]
    if controlled_sample.get("entity_id") != 1 or controlled_sample.get("spatial_epoch") != 1:
        raise SystemExit(f"unexpected controlled movement sample identity/epoch: {controlled_sample}")
    require_close_vector(
        controlled_sample.get("position_m"),
        [0.001, 0.0, 0.0],
        "controlled movement sample position",
    )
    require_close_vector(
        controlled_sample.get("velocity_mps"),
        [0.1, 0.0, 0.0],
        "controlled movement sample velocity",
    )

    npc_sample = samples[1]
    if npc_sample.get("entity_id") != 2 or npc_sample.get("spatial_epoch") != 1:
        raise SystemExit(f"unexpected NPC movement sample identity/epoch: {npc_sample}")
    require_close_vector(
        npc_sample.get("position_m"),
        [2.999, 0.0, -3.0],
        "living-need NPC movement sample position",
    )
    require_close_vector(
        npc_sample.get("velocity_mps"),
        [-0.1, 0.0, 0.0],
        "living-need NPC movement sample velocity",
    )

    expected_presentation = {
        "controlled_entity_id": 1,
        "last_tick": 1,
        "last_revision": 4,
        "protocol_version": 6,
        "observed_entity_ids": [1, 2],
        "bound_entity_ids": [1, 2],
        "visible_bound_entity_ids": [1, 2],
        "controlled_spatial_initialized": True,
        "controlled_spatial_epoch": 1,
        "controlled_spatial_tick": 1,
        "controlled_spatial_revision": 4,
        "movement_batches_applied": 1,
    }
    for key, value in expected_presentation.items():
        if presentation.get(key) != value:
            raise SystemExit(f"unexpected presentation {key}: expected {value}, got {presentation.get(key)}")
    require_close_vector(
        presentation.get("controlled_authoritative_position_m"),
        [0.001, 0.0, 0.0],
        "presentation authoritative position",
    )
    require_close_vector(
        presentation.get("controlled_authoritative_velocity_mps"),
        [0.1, 0.0, 0.0],
        "presentation authoritative velocity",
    )

    expected_localized_text = {
        "ru": {
            "hud_title": "Диагностика выполнения",
            "controls_hint": "WASD / стик — движение  ·  Shift — спринт  ·  мышь / правый стик — обзор  ·  Esc — курсор",
        },
        "en": {
            "hud_title": "Runtime diagnostics",
            "controls_hint": "WASD / stick move  ·  Shift sprint  ·  mouse / right stick look  ·  Esc releases pointer",
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
        "--audio-driver",
        SMOKE_AUDIO_DRIVER,
        "--disable-vsync",
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
