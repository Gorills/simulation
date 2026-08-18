#!/usr/bin/env python3
from __future__ import annotations

from datetime import datetime, timezone
import fcntl
import json
import os
from pathlib import Path
import re
import shutil
import signal
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
CACHE = ROOT / ".cache" / "graphics-check"
LOCK = CACHE / "graphics.lock"
SMOKE = ROOT / "build" / "native-debug" / "platform_graphics_smoke"
KEY_SENDER = ROOT / "build" / "native-debug" / "platform_x11_key_sender"
CAPTURE = ROOT / "build" / "native-debug" / "platform_x11_capture"
WINDOW_TITLE = "Simulation Graphics Stack Smoke"
HARD_TIMEOUT_SECONDS = 15.0


def require_tool(name: str) -> str:
    path = shutil.which(name)
    if path is None:
        raise RuntimeError(f"required graphics-check tool is missing: {name}")
    return path


def terminate_group(process: subprocess.Popen[object] | None) -> None:
    if process is None or process.poll() is not None:
        return
    try:
        os.killpg(process.pid, signal.SIGTERM)
    except ProcessLookupError:
        return
    try:
        process.wait(timeout=1)
        return
    except subprocess.TimeoutExpired:
        pass
    try:
        os.killpg(process.pid, signal.SIGKILL)
    except ProcessLookupError:
        pass
    try:
        process.wait(timeout=1)
    except subprocess.TimeoutExpired:
        pass


def choose_display() -> tuple[str, Path]:
    socket_dir = Path("/tmp/.X11-unix")
    for number in range(90, 120):
        socket_path = socket_dir / f"X{number}"
        if not socket_path.exists():
            return f":{number}", socket_path
    raise RuntimeError("no free Xvfb display in reserved range 90..119")


def wait_until(predicate, *, deadline: float, description: str) -> None:
    while time.monotonic() < deadline:
        if predicate():
            return
        time.sleep(0.05)
    raise RuntimeError(f"timed out waiting for {description}")


def find_window_id(xwininfo: str, display: str) -> str | None:
    result = subprocess.run(
        [xwininfo, "-display", display, "-root", "-tree"],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
        timeout=2,
    )
    pattern = re.compile(rf"^\s*(0x[0-9a-fA-F]+)\s+\"{re.escape(WINDOW_TITLE)}\"", re.MULTILINE)
    match = pattern.search(result.stdout)
    return None if match is None else match.group(1)


def run() -> Path:
    if not sys.platform.startswith("linux"):
        raise RuntimeError("headless graphics-check is currently implemented for the Linux agent host only")
    if not SMOKE.is_file() or not KEY_SENDER.is_file() or not CAPTURE.is_file():
        raise RuntimeError("graphics targets are not built; run `python tools/dev.py build` first")

    xvfb = require_tool("Xvfb")
    xwininfo = require_tool("xwininfo")
    image_convert = shutil.which("magick") or shutil.which("convert")
    if image_convert is None:
        raise RuntimeError("required graphics-check tool is missing: magick/convert")

    CACHE.mkdir(parents=True, exist_ok=True)
    LOCK.touch(exist_ok=True)
    with LOCK.open("r+", encoding="utf-8") as lock_file:
        try:
            fcntl.flock(lock_file.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError as error:
            raise RuntimeError("GRAPHICS CHECK BUSY") from error

        run_id = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S.%fZ")
        run_dir = CACHE / run_id
        run_dir.mkdir(parents=True)
        state_file = run_dir / "state.txt"
        stdout_file = run_dir / "stdout.log"
        stderr_file = run_dir / "stderr.log"
        capture_ppm = run_dir / "final.ppm"
        screenshot = run_dir / "final.png"
        display, display_socket = choose_display()
        deadline = time.monotonic() + HARD_TIMEOUT_SECONDS
        xvfb_process: subprocess.Popen[object] | None = None
        smoke_process: subprocess.Popen[object] | None = None
        xvfb_stderr_file = run_dir / "xvfb.stderr.log"
        status = "failure"
        started = time.monotonic()
        env = os.environ.copy()
        env["DISPLAY"] = display

        try:
            with xvfb_stderr_file.open("wb") as xvfb_stderr:
                xvfb_process = subprocess.Popen(
                    [xvfb, display, "-screen", "0", "640x480x24", "-nolisten", "tcp"],
                    cwd=ROOT,
                    stdout=subprocess.DEVNULL,
                    stderr=xvfb_stderr,
                    start_new_session=True,
                )
                wait_until(lambda: display_socket.exists(), deadline=deadline, description="Xvfb socket")
                if xvfb_process.poll() is not None:
                    raise RuntimeError(f"Xvfb exited early with {xvfb_process.returncode}")

            with stdout_file.open("w", encoding="utf-8") as stdout, stderr_file.open("w", encoding="utf-8") as stderr:
                smoke_process = subprocess.Popen(
                    [str(SMOKE), "--state-file", str(state_file)],
                    cwd=ROOT,
                    env=env,
                    stdout=stdout,
                    stderr=stderr,
                    start_new_session=True,
                    text=True,
                )

                window_id: str | None = None
                while time.monotonic() < deadline and window_id is None:
                    if smoke_process.poll() is not None:
                        raise RuntimeError(f"graphics smoke exited early with {smoke_process.returncode}")
                    window_id = find_window_id(xwininfo, display)
                    if window_id is None:
                        time.sleep(0.05)
                if window_id is None:
                    raise RuntimeError("timed out waiting for graphics smoke window")

                subprocess.run(
                    [str(KEY_SENDER), window_id, "D"],
                    cwd=ROOT,
                    env=env,
                    check=True,
                    timeout=2,
                )
                wait_until(state_file.exists, deadline=deadline, description="real D key event")
                if state_file.read_text(encoding="utf-8") != "d_received=1\n":
                    raise RuntimeError("graphics smoke state evidence is invalid")

                subprocess.run(
                    [str(CAPTURE), window_id, str(capture_ppm)],
                    cwd=ROOT,
                    env=env,
                    check=True,
                    timeout=3,
                )
                ppm = capture_ppm.read_bytes()
                header_end = ppm.find(b"\n255\n")
                if header_end < 0:
                    raise RuntimeError("captured PPM header is invalid")
                header = ppm[: header_end + 5].decode("ascii")
                dimensions = header.splitlines()[1].split()
                width, height = (int(dimensions[0]), int(dimensions[1]))
                if (width, height) != (320, 200):
                    raise RuntimeError(f"captured window size mismatch: {(width, height)}")
                pixels = ppm[header_end + 5 :]
                probe_offset = (100 * width + 228) * 3
                probe = tuple(pixels[probe_offset : probe_offset + 3])
                if probe != (0x35, 0xA8, 0x53):
                    raise RuntimeError(f"captured framebuffer probe mismatch: {probe}")
                converter_argv = [image_convert, str(capture_ppm), str(screenshot)]
                subprocess.run(converter_argv, cwd=ROOT, check=True, timeout=3)
                if not screenshot.is_file() or screenshot.stat().st_size == 0:
                    raise RuntimeError("PNG conversion did not produce a screenshot")

                if smoke_process.poll() is not None:
                    raise RuntimeError(
                        f"graphics smoke exited before harness-owned teardown with {smoke_process.returncode}"
                    )

                # The harness owns fixture lifetime. Input verification is already proven by the
                # real D key event above; teardown must not depend on another UI event or a
                # stale window id. Terminate only the process group created by this run.
                terminate_group(smoke_process)
                if smoke_process.poll() is None:
                    raise RuntimeError("graphics smoke did not terminate under harness ownership")
                status = "success"
        finally:
            terminate_group(smoke_process)
            terminate_group(xvfb_process)
            (run_dir / "run.json").write_text(
                json.dumps(
                    {
                        "status": status,
                        "display": display,
                        "elapsedSeconds": round(time.monotonic() - started, 3),
                        "windowTitle": WINDOW_TITLE,
                        "input": "D",
                        "stateEvidence": "d_received=1",
                        "renderProbe": "#35A853 at (228,100)",
                        "capture": "final.ppm -> final.png",
                        "teardown": "harness-owned process-group termination",
                    },
                    indent=2,
                )
                + "\n",
                encoding="utf-8",
            )

        if status != "success":
            raise RuntimeError(f"graphics stack verification failed; artifacts: {run_dir}")
        print(f"GRAPHICS CHECK SUCCESS {run_dir}")
        return run_dir


def main() -> int:
    try:
        run()
        return 0
    except (OSError, RuntimeError, subprocess.CalledProcessError, subprocess.TimeoutExpired) as error:
        print(f"GRAPHICS CHECK ERROR: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
