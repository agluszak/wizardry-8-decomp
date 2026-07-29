"""Focus-safe X display selection for Wine runtime products."""

from __future__ import annotations

import os
import shutil
import subprocess
import time
from collections.abc import Iterator
from contextlib import contextmanager
from pathlib import Path

# SurRender changes to the mode selected by config/runtime/3DVideo.CFG and
# rejects a virtual X server that does not expose that exact screen mode.
_SCREEN_GEOMETRY = "640x480x16"
_STARTUP_TIMEOUT_SECONDS = 10.0


@contextmanager
def runtime_display(
    environment: dict[str, str],
    *,
    default: str,
    log_path: Path,
) -> Iterator[str | None]:
    """Select a host, existing, or private off-screen display.

    Runtime tests default to ``virtual`` so Wine cannot map onto the inherited
    desktop. Interactive runs default to ``host`` but accept the same
    ``WIZ8_RUNTIME_DISPLAY=virtual`` opt-in for unattended/background use.
    """

    requested = os.environ.get("WIZ8_RUNTIME_DISPLAY", default).strip() or default
    if requested == "host":
        yield None
        return
    if requested.startswith(":"):
        previous = environment.get("DISPLAY")
        environment["DISPLAY"] = requested
        try:
            yield requested
        finally:
            _restore_display(environment, previous)
        return
    if requested != "virtual":
        raise RuntimeError(
            "WIZ8_RUNTIME_DISPLAY must be 'virtual', 'host', or a display like ':5'; "
            f"got {requested!r}"
        )

    binary = shutil.which("Xvfb")
    if binary is None:
        raise RuntimeError(
            "WIZ8_RUNTIME_DISPLAY=virtual requires Xvfb on PATH; "
            "use WIZ8_RUNTIME_DISPLAY=host explicitly for the desktop"
        )

    log_path.parent.mkdir(parents=True, exist_ok=True)
    previous = environment.get("DISPLAY")
    read_fd, write_fd = os.pipe()
    process: subprocess.Popen[bytes] | None = None
    with log_path.open("wb") as log:
        try:
            os.set_inheritable(write_fd, True)
            process = subprocess.Popen(
                [
                    binary,
                    "-displayfd",
                    str(write_fd),
                    "-screen",
                    "0",
                    _SCREEN_GEOMETRY,
                    "-nolisten",
                    "tcp",
                ],
                pass_fds=(write_fd,),
                stdout=log,
                stderr=subprocess.STDOUT,
            )
            os.close(write_fd)
            write_fd = -1
            number = _read_display_number(read_fd, process)
            if number is None:
                raise RuntimeError(f"Xvfb failed to start; see {log_path}")
            display = f":{number}"
            environment["DISPLAY"] = display
            yield display
        finally:
            _restore_display(environment, previous)
            if write_fd != -1:
                os.close(write_fd)
            os.close(read_fd)
            if process is not None and process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()


def _restore_display(environment: dict[str, str], previous: str | None) -> None:
    if previous is None:
        environment.pop("DISPLAY", None)
    else:
        environment["DISPLAY"] = previous


def _read_display_number(read_fd: int, process: subprocess.Popen[bytes]) -> str | None:
    deadline = time.monotonic() + _STARTUP_TIMEOUT_SECONDS
    collected = b""
    os.set_blocking(read_fd, False)
    while time.monotonic() < deadline:
        if process.poll() is not None:
            return None
        try:
            chunk = os.read(read_fd, 32)
        except BlockingIOError:
            time.sleep(0.05)
            continue
        if chunk:
            collected += chunk
            if b"\n" in collected:
                number = collected.split(b"\n", 1)[0].decode("ascii", "ignore").strip()
                return number or None
        else:
            time.sleep(0.05)
    return None
