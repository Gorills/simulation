#!/usr/bin/env python3
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import fcntl
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
CACHE = ROOT / ".cache" / "play"
LOCK_PATH = CACHE / "terminal.lock"
GAME = ROOT / "build" / "native-debug" / "sim_cli"
HARD_TIMEOUT_SECONDS = 10
GRACE_SECONDS = 1
DEBUG_PREFIX = "DEBUG_JSON "


def terminate_own_process_group(group_id: int) -> None:
    try:
        os.killpg(group_id, signal.SIGTERM)
    except ProcessLookupError:
        return
    deadline = time.monotonic() + GRACE_SECONDS
    while time.monotonic() < deadline:
        try:
            os.killpg(group_id, 0)
        except ProcessLookupError:
            return
        time.sleep(0.05)
    try:
        os.killpg(group_id, signal.SIGKILL)
    except ProcessLookupError:
        pass


def retain_successes(limit: int = 5) -> None:
    successes: list[Path] = []
    for child in CACHE.iterdir():
        if not child.is_dir() or not (child / "run.json").exists():
            continue
        try:
            data = json.loads((child / "run.json").read_text(encoding="utf-8"))
        except (json.JSONDecodeError, OSError):
            continue
        if data.get("status") == "success":
            successes.append(child)
    successes.sort(key=lambda path: path.name, reverse=True)
    for stale in successes[limit:]:
        for file in stale.iterdir():
            file.unlink()
        stale.rmdir()


def parse_debug_states(stdout: str) -> list[dict[str, object]]:
    states: list[dict[str, object]] = []
    for line in stdout.splitlines():
        if line.startswith(DEBUG_PREFIX):
            states.append(json.loads(line.removeprefix(DEBUG_PREFIX)))
    return states


def final_frame(stdout: str) -> str:
    frames = re.findall(r"FRAME_BEGIN\n(.*?)FRAME_END", stdout, flags=re.DOTALL)
    if not frames:
        raise RuntimeError("playtest produced no rendered frame")
    return frames[-1].rstrip() + "\n"


def verify_smoke(stdout: str) -> dict[str, object]:
    states = parse_debug_states(stdout)
    if len(states) < 2:
        raise RuntimeError("playtest did not expose before/after debug states")
    before = states[0]
    after = states[-1]
    before_player = before.get("player")
    after_player = after.get("player")
    if not isinstance(before_player, dict) or not isinstance(after_player, dict):
        raise RuntimeError("debug state has no player projection")
    if int(after_player["x"]) != int(before_player["x"]) + 1:
        raise RuntimeError("D input did not move authoritative x by +1")
    if int(after_player["y"]) != int(before_player["y"]):
        raise RuntimeError("D input unexpectedly changed authoritative y")
    if int(after_player["tick"]) != int(before_player["tick"]) + 1:
        raise RuntimeError("D input did not advance authoritative tick")
    last = after.get("lastCommandResult")
    if not isinstance(last, dict) or last.get("intent") != "MoveIntent" or last.get("accepted") is not True:
        raise RuntimeError("debug state does not prove accepted MoveIntent")
    return after


def main() -> int:
    parser = argparse.ArgumentParser(description="Singleton bounded native playtest")
    parser.add_argument("--scenario", choices=("smoke",), required=True)
    args = parser.parse_args()

    if not GAME.exists():
        print("PLAYTEST SETUP REQUIRED: run `python tools/dev.py build`", file=os.sys.stderr)
        return 3

    CACHE.mkdir(parents=True, exist_ok=True)
    with LOCK_PATH.open("a+", encoding="utf-8") as lock_file:
        try:
            fcntl.flock(lock_file.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            print("PLAYTEST BUSY", file=os.sys.stderr)
            return 2

        run_id = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S.%fZ")
        run_dir = CACHE / run_id
        run_dir.mkdir(parents=True)
        started = time.monotonic()
        process = subprocess.Popen(
            [str(GAME)],
            cwd=ROOT,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            start_new_session=True,
        )

        status = "failure"
        return_code = 1
        stdout = ""
        stderr = ""
        try:
            stdout, stderr = process.communicate(input="d\nq\n", timeout=HARD_TIMEOUT_SECONDS)
            if process.returncode != 0:
                raise RuntimeError(f"sim_cli exited with {process.returncode}")
            final_debug = verify_smoke(stdout)
            (run_dir / "final.txt").write_text(final_frame(stdout), encoding="utf-8")
            (run_dir / "debug.json").write_text(json.dumps(final_debug, indent=2) + "\n", encoding="utf-8")
            status = "success"
            return_code = 0
        except subprocess.TimeoutExpired:
            terminate_own_process_group(process.pid)
            stdout, stderr = process.communicate()
            stderr += "\nPLAYTEST TIMEOUT\n"
            status = "timeout"
            return_code = 124
        except (RuntimeError, json.JSONDecodeError, KeyError, TypeError, ValueError) as error:
            stderr += f"\nPLAYTEST VERIFICATION ERROR: {error}\n"
            status = "failure"
            return_code = 1
        finally:
            (run_dir / "stdout.log").write_text(stdout, encoding="utf-8")
            (run_dir / "stderr.log").write_text(stderr, encoding="utf-8")
            elapsed = time.monotonic() - started
            (run_dir / "run.json").write_text(
                json.dumps({
                    "scenario": args.scenario,
                    "status": status,
                    "returnCode": return_code,
                    "elapsedSeconds": round(elapsed, 3),
                }, indent=2) + "\n",
                encoding="utf-8",
            )
            if status == "success":
                retain_successes()

        print(f"PLAYTEST {status.upper()} {run_dir}")
        return return_code


if __name__ == "__main__":
    raise SystemExit(main())
