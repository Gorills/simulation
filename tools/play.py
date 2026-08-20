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
SUPPORTED_SCENARIOS = ("smoke", "offscreen", "rest_interference")
SMOKE_AUDIO_DRIVER = "Dummy"
PROTOCOL_VERSION = 7
OBSERVED_ENTITY_IDS = [1, 2, 3]


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


def load_playtest_artifact(path: Path) -> dict[str, object]:
    debug_path = path / "debug.json"
    screenshot_path = path / "final.png"
    if not debug_path.is_file() or not screenshot_path.is_file():
        raise SystemExit(f"playtest did not produce required artifacts in {path}")

    evidence = json.loads(debug_path.read_text(encoding="utf-8"))
    if not isinstance(evidence, dict):
        raise SystemExit("debug artifact must be a JSON object")
    return evidence


def movement_sample(batch: object, entity_id: int, label: str) -> dict[str, object]:
    if not isinstance(batch, dict):
        raise SystemExit(f"{label} must be a movement batch object")
    samples = batch.get("samples")
    if not isinstance(samples, list):
        raise SystemExit(f"{label} is missing movement samples")
    for sample in samples:
        if isinstance(sample, dict) and sample.get("entity_id") == entity_id:
            return sample
    raise SystemExit(f"{label} is missing EntityId {entity_id}")


def validate_smoke_artifact(path: Path, expected_locale: str) -> dict[str, object]:
    evidence = load_playtest_artifact(path)

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
        "seed": 1,
        "protocol_version": PROTOCOL_VERSION,
    }
    for key, value in expected_bootstrap.items():
        if bootstrap.get(key) != value:
            raise SystemExit(f"unexpected bootstrap projection {key}: expected {value}, got {bootstrap.get(key)}")
    bootstrap_revision = bootstrap.get("revision")
    if not isinstance(bootstrap_revision, int):
        raise SystemExit(f"unexpected bootstrap projection revision: {bootstrap_revision}")

    if movement_stream.get("duplicate_batch_rejected") is not True:
        raise SystemExit("WorldPresentation did not prove duplicate movement-batch rejection")
    batch = movement_stream.get("batch")
    if not isinstance(batch, dict):
        raise SystemExit("movement stream is missing its authoritative batch")
    batch_tick = batch.get("tick")
    batch_revision = batch.get("revision")
    if not isinstance(batch_tick, int) or not isinstance(batch_revision, int):
        raise SystemExit("movement batch has invalid temporal header")
    if batch_tick != 1 or batch_revision != bootstrap_revision + 1:
        raise SystemExit(
            "movement batch did not advance one locomotion tick/revision after bootstrap"
        )
    if batch.get("protocol_version") != PROTOCOL_VERSION:
        raise SystemExit(f"unexpected movement batch protocol version: {batch.get('protocol_version')}")

    expected_observed = {
        "controlled_actor_id": 1,
        "tick": batch_tick,
        "revision": batch_revision,
        "protocol_version": PROTOCOL_VERSION,
    }
    for key, value in expected_observed.items():
        if observed.get(key) != value:
            raise SystemExit(f"unexpected observed-world {key}: expected {value}, got {observed.get(key)}")
    expected_entities = [{"entity_id": entity_id} for entity_id in OBSERVED_ENTITY_IDS]
    if observed.get("entities") != expected_entities:
        raise SystemExit(f"unexpected observed entities: {observed.get('entities')}")

    expected_spatial_scalars = {
        "entity_id": 1,
        "spatial_epoch": 1,
        "tick": batch_tick,
        "revision": batch_revision,
        "protocol_version": PROTOCOL_VERSION,
    }
    for key, value in expected_spatial_scalars.items():
        if spatial.get(key) != value:
            raise SystemExit(f"unexpected controlled spatial {key}: expected {value}, got {spatial.get(key)}")
    require_close_vector(spatial.get("position_m"), [0.001, 0.0, 0.0], "controlled spatial position")
    require_close_vector(spatial.get("velocity_mps"), [0.1, 0.0, 0.0], "controlled spatial velocity")

    samples = batch.get("samples")
    if (
        not isinstance(samples, list)
        or len(samples) != 3
        or not all(isinstance(sample, dict) for sample in samples)
    ):
        raise SystemExit(f"unexpected movement samples: {samples}")

    controlled_sample = movement_sample(batch, 1, "smoke batch")
    if controlled_sample.get("spatial_epoch") != 1:
        raise SystemExit(f"unexpected controlled movement sample epoch: {controlled_sample}")
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

    npc_sample = movement_sample(batch, 2, "smoke batch")
    if npc_sample.get("spatial_epoch") != 1:
        raise SystemExit(f"unexpected NPC movement sample epoch: {npc_sample}")
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

    idle_npc_sample = movement_sample(batch, 3, "smoke batch")
    if idle_npc_sample.get("spatial_epoch") != 1:
        raise SystemExit(f"unexpected idle NPC movement sample epoch: {idle_npc_sample}")
    require_close_vector(
        idle_npc_sample.get("position_m"),
        [2.0, 0.0, 2.0],
        "idle NPC movement sample position",
    )
    require_close_vector(
        idle_npc_sample.get("velocity_mps"),
        [0.0, 0.0, 0.0],
        "idle NPC movement sample velocity",
    )

    expected_presentation = {
        "controlled_entity_id": 1,
        "last_tick": batch_tick,
        "last_revision": batch_revision,
        "protocol_version": PROTOCOL_VERSION,
        "observed_entity_ids": OBSERVED_ENTITY_IDS,
        "bound_entity_ids": OBSERVED_ENTITY_IDS,
        "visible_bound_entity_ids": OBSERVED_ENTITY_IDS,
        "controlled_spatial_initialized": True,
        "controlled_spatial_epoch": 1,
        "controlled_spatial_tick": batch_tick,
        "controlled_spatial_revision": batch_revision,
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


def validate_offscreen_artifact(path: Path, expected_locale: str) -> dict[str, object]:
    evidence = load_playtest_artifact(path)
    offscreen = evidence.get("offscreen_continuation")
    presentation = evidence.get("presentation")
    localization = evidence.get("localization")
    movement_stream = evidence.get("movement_stream")
    if not all(isinstance(section, dict) for section in (offscreen, presentation, localization, movement_stream)):
        raise SystemExit("offscreen artifact is missing offscreen/presentation/localization/movement sections")

    assert isinstance(offscreen, dict)
    assert isinstance(presentation, dict)
    assert isinstance(localization, dict)
    assert isinstance(movement_stream, dict)

    if offscreen.get("entity_id") != 2:
        raise SystemExit(f"unexpected offscreen entity: {offscreen.get('entity_id')}")
    for key in (
        "observed_while_absent",
        "presentation_absent_before_tick",
        "presentation_absent_after_tick",
        "rematerialized_hidden_before_sample",
        "rematerialized_visible_after_sample",
    ):
        if offscreen.get(key) is not True:
            raise SystemExit(f"offscreen continuation did not prove {key}")

    first_batch = offscreen.get("first_batch")
    absent_batch = offscreen.get("offscreen_batch")
    final_batch = movement_stream.get("batch")
    if not all(isinstance(batch, dict) for batch in (first_batch, absent_batch, final_batch)):
        raise SystemExit("offscreen continuation is missing an authoritative movement batch")
    assert isinstance(first_batch, dict)
    assert isinstance(absent_batch, dict)
    assert isinstance(final_batch, dict)

    first_tick = first_batch.get("tick")
    absent_tick = absent_batch.get("tick")
    final_tick = final_batch.get("tick")
    first_revision = first_batch.get("revision")
    absent_revision = absent_batch.get("revision")
    final_revision = final_batch.get("revision")
    if not all(isinstance(value, int) for value in (
        first_tick,
        absent_tick,
        final_tick,
        first_revision,
        absent_revision,
        final_revision,
    )):
        raise SystemExit("offscreen movement batches have invalid temporal headers")
    assert isinstance(first_tick, int)
    assert isinstance(absent_tick, int)
    assert isinstance(final_tick, int)
    assert isinstance(first_revision, int)
    assert isinstance(absent_revision, int)
    assert isinstance(final_revision, int)
    if absent_tick != first_tick + 1 or final_tick != absent_tick + 1:
        raise SystemExit("offscreen movement ticks are not consecutive")
    if absent_revision != first_revision + 1 or final_revision != absent_revision + 1:
        raise SystemExit("offscreen movement revisions are not consecutive")
    if not all(
        batch.get("protocol_version") == PROTOCOL_VERSION
        for batch in (first_batch, absent_batch, final_batch)
    ):
        raise SystemExit("offscreen movement protocol version mismatch")

    first_npc = movement_sample(first_batch, 2, "first batch")
    absent_npc = movement_sample(absent_batch, 2, "offscreen batch")
    final_npc = movement_sample(final_batch, 2, "final batch")
    first_position = first_npc.get("position_m")
    absent_position = absent_npc.get("position_m")
    final_position = final_npc.get("position_m")
    if not all(isinstance(position, list) and len(position) == 3 for position in (
        first_position,
        absent_position,
        final_position,
    )):
        raise SystemExit("offscreen NPC samples have invalid positions")
    assert isinstance(first_position, list)
    assert isinstance(absent_position, list)
    assert isinstance(final_position, list)
    if not (
        isinstance(first_position[0], (int, float))
        and isinstance(absent_position[0], (int, float))
        and isinstance(final_position[0], (int, float))
        and float(absent_position[0]) < float(first_position[0])
        and float(final_position[0]) < float(absent_position[0])
    ):
        raise SystemExit("living-need NPC did not continue authoritative travel while unmaterialized")

    expected_presentation = {
        "observed_entity_ids": OBSERVED_ENTITY_IDS,
        "bound_entity_ids": OBSERVED_ENTITY_IDS,
        "visible_bound_entity_ids": OBSERVED_ENTITY_IDS,
        "last_tick": final_tick,
        "last_revision": final_revision,
        "controlled_spatial_tick": final_tick,
        "controlled_spatial_revision": final_revision,
        "movement_batches_applied": 3,
    }
    for key, value in expected_presentation.items():
        if presentation.get(key) != value:
            raise SystemExit(f"unexpected offscreen presentation {key}: expected {value}, got {presentation.get(key)}")

    if localization.get("locale") != expected_locale:
        raise SystemExit(
            f"unexpected offscreen locale: expected {expected_locale}, got {localization.get('locale')}"
        )
    return evidence


def validate_rest_interference_artifact(path: Path, expected_locale: str) -> dict[str, object]:
    evidence = load_playtest_artifact(path)
    if not (path / "blocked.png").is_file():
        raise SystemExit("rest interference scenario did not capture blocked.png")

    interaction = evidence.get("rest_interference")
    living_need = evidence.get("living_need_projection")
    localization = evidence.get("localization")
    presentation = evidence.get("presentation")
    movement_stream = evidence.get("movement_stream")
    if not all(
        isinstance(section, dict)
        for section in (interaction, living_need, localization, presentation, movement_stream)
    ):
        raise SystemExit("rest interference artifact is missing interaction/need/localization/presentation/movement sections")

    assert isinstance(interaction, dict)
    assert isinstance(living_need, dict)
    assert isinstance(localization, dict)
    assert isinstance(presentation, dict)
    assert isinstance(movement_stream, dict)

    if interaction.get("entity_id") != 2 or interaction.get("initial_status") != "traveling":
        raise SystemExit("rest interference did not begin with the expected traveling NPC need")
    blocked = interaction.get("blocked_projection")
    satisfied = interaction.get("satisfied_projection")
    if not isinstance(blocked, dict) or not isinstance(satisfied, dict):
        raise SystemExit("rest interference is missing blocked/satisfied projections")
    if blocked.get("entity_id") != 2 or blocked.get("status") != "blocked":
        raise SystemExit(f"unexpected blocked need projection: {blocked}")
    if satisfied.get("entity_id") != 2 or satisfied.get("status") != "satisfied":
        raise SystemExit(f"unexpected satisfied need projection: {satisfied}")
    if (
        blocked.get("protocol_version") != PROTOCOL_VERSION
        or satisfied.get("protocol_version") != PROTOCOL_VERSION
    ):
        raise SystemExit("rest interference need projection protocol mismatch")

    blocked_tick = blocked.get("tick")
    satisfied_tick = satisfied.get("tick")
    blocked_revision = blocked.get("revision")
    satisfied_revision = satisfied.get("revision")
    if not all(isinstance(value, int) for value in (
        blocked_tick,
        satisfied_tick,
        blocked_revision,
        satisfied_revision,
    )):
        raise SystemExit("rest interference need projections have invalid temporal fields")
    assert isinstance(blocked_tick, int)
    assert isinstance(satisfied_tick, int)
    assert isinstance(blocked_revision, int)
    assert isinstance(satisfied_revision, int)
    if satisfied_tick <= blocked_tick or satisfied_revision <= blocked_revision:
        raise SystemExit("rest interference did not progress from blocked to a later satisfied state")

    blocked_position = interaction.get("blocked_player_position_m")
    satisfied_position = interaction.get("satisfied_player_position_m")
    if not all(isinstance(position, list) and len(position) == 2 for position in (
        blocked_position,
        satisfied_position,
    )):
        raise SystemExit("rest interference has invalid controlled-actor positions")
    assert isinstance(blocked_position, list)
    assert isinstance(satisfied_position, list)
    if not all(isinstance(value, (int, float)) for value in blocked_position + satisfied_position):
        raise SystemExit("rest interference controlled-actor positions must be numeric")
    if abs(float(blocked_position[0]) + 3.0) > 0.15 or abs(float(blocked_position[1]) + 3.0) > 0.15:
        raise SystemExit(f"controlled actor was not inside the rest tolerance when blocked: {blocked_position}")
    if (
        abs(float(satisfied_position[0]) + 3.0) <= 0.15
        and abs(float(satisfied_position[1]) + 3.0) <= 0.15
    ):
        raise SystemExit(f"controlled actor did not leave the rest tolerance before satisfaction: {satisfied_position}")

    localized_status = {
        "ru": {
            "blocked": "Место отдыха занято",
            "satisfied": "Место отдыха доступно",
        },
        "en": {
            "blocked": "Rest place blocked",
            "satisfied": "Rest place available",
        },
    }
    if interaction.get("blocked_hud_text") != localized_status[expected_locale]["blocked"]:
        raise SystemExit("blocked HUD feedback did not match the active locale")
    if localization.get("locale") != expected_locale:
        raise SystemExit(f"unexpected rest interference locale: {localization.get('locale')}")
    if localization.get("living_need_status_text") != localized_status[expected_locale]["satisfied"]:
        raise SystemExit("final satisfied HUD feedback did not match the active locale")

    if living_need.get("entity_id") != 2 or living_need.get("status") != "satisfied":
        raise SystemExit(f"final living-need projection is not satisfied: {living_need}")
    if living_need.get("protocol_version") != PROTOCOL_VERSION:
        raise SystemExit("final living-need projection protocol mismatch")
    if living_need.get("tick") != presentation.get("last_tick"):
        raise SystemExit("final need projection tick does not match presented world tick")
    batch = movement_stream.get("batch")
    if not isinstance(batch, dict) or batch.get("tick") != living_need.get("tick"):
        raise SystemExit("final movement batch does not align with the satisfied need projection")
    if batch.get("protocol_version") != PROTOCOL_VERSION:
        raise SystemExit("final movement batch protocol mismatch")
    batch_tick = batch.get("tick")
    batch_revision = batch.get("revision")
    presentation_revision = presentation.get("last_revision")
    controlled_spatial_tick = presentation.get("controlled_spatial_tick")
    controlled_spatial_revision = presentation.get("controlled_spatial_revision")
    living_revision = living_need.get("revision")
    if not all(
        isinstance(value, int)
        for value in (
            batch_tick,
            batch_revision,
            presentation_revision,
            controlled_spatial_tick,
            controlled_spatial_revision,
            living_revision,
        )
    ):
        raise SystemExit("final rest-interference temporal fields are invalid")
    assert isinstance(batch_tick, int)
    assert isinstance(batch_revision, int)
    assert isinstance(presentation_revision, int)
    assert isinstance(controlled_spatial_tick, int)
    assert isinstance(controlled_spatial_revision, int)
    assert isinstance(living_revision, int)
    if controlled_spatial_tick != batch_tick or controlled_spatial_revision != batch_revision:
        raise SystemExit("controlled spatial presentation does not match the final movement batch")
    if presentation_revision != living_revision:
        raise SystemExit("latest presented world revision does not match the final living-need read")
    if living_revision not in (batch_revision, batch_revision + 1):
        raise SystemExit(
            "final need revision is not aligned with movement or one post-movement resource transition"
        )
    return evidence


def main() -> int:
    parser = argparse.ArgumentParser(description="Run one bounded Godot playtest owned by this repository")
    parser.add_argument("--scenario", default="smoke", choices=SUPPORTED_SCENARIOS)
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

    if args.scenario == "offscreen":
        evidence = validate_offscreen_artifact(artifact_dir, args.locale)
    elif args.scenario == "rest_interference":
        evidence = validate_rest_interference_artifact(artifact_dir, args.locale)
    else:
        evidence = validate_smoke_artifact(artifact_dir, args.locale)
    metadata.update({"status": "passed", "evidence": evidence})
    run_path.write_text(json.dumps(metadata, indent=2, ensure_ascii=False), encoding="utf-8")
    print(f"PLAYTEST PASSED ({args.scenario}, {args.locale}): {artifact_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
