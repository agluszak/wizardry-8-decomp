"""Open an interactive GDB session against the windowed runtime."""

from __future__ import annotations

import os
import subprocess
import sys
import time
from pathlib import Path

from .dynamic import _allocate_port, _listening, _terminate_process_group
from .runtime import _map_functions


def main() -> int:
    repository = Path(__file__).resolve().parents[2]
    executable = repository / "build" / "decomp" / "Wiz8Runtime.exe"
    work_dir = os.environ.get("WIZ8_WORK_DIR")
    if not executable.is_file():
        raise SystemExit(f"Missing {executable} — run 'just build runtime' first.")
    if not work_dir:
        raise SystemExit("Set WIZ8_WORK_DIR in .env or the environment.")
    game_dir = Path(work_dir) / "variants" / "gog-base"
    if not (game_dir / "Data").is_dir():
        raise SystemExit(f"Missing retail game data in {game_dir}.")

    runtime_executable = game_dir / executable.name
    if runtime_executable.exists() and not runtime_executable.is_symlink():
        raise SystemExit(f"Refusing to replace unmanaged {runtime_executable}.")
    runtime_executable.unlink(missing_ok=True)
    runtime_executable.symlink_to(executable)
    sound_init = next(
        function.address
        for function in _map_functions(executable.with_suffix(".map"))
        if "SoundInitHardware00409C50" in function.symbol
    )

    environment = {**os.environ}
    environment.setdefault("WINEPREFIX", str(Path(work_dir) / "wine" / "wiz8-runtime"))
    environment.setdefault("WINEDEBUG", "-all")
    port = _allocate_port()
    proxy = subprocess.Popen(
        [
            "winedbg",
            "--gdb",
            "--no-start",
            "--port",
            str(port),
            f"./{runtime_executable.name}",
            "/WINDOW",
            *sys.argv[1:],
        ],
        cwd=game_dir,
        env=environment,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        start_new_session=True,
    )
    try:
        if not _listening(port, time.monotonic() + 60):
            raise RuntimeError("winedbg --gdb did not open its port")
        debugger = subprocess.Popen(
            [
                "gdb",
                "-nx",
                "-q",
                "-ex",
                "set pagination off",
                "-ex",
                f"target remote localhost:{port}",
                "-ex",
                f"set $SoundInitHardware00409C50 = {sound_init:#x}",
                "-ex",
                "break *$SoundInitHardware00409C50",
                "-ex",
                "break 'mss32!_AIL_open_digital_driver@16'",
                "-ex",
                "break 'mss32!_AIL_digital_configuration@16'",
            ],
            cwd=game_dir,
            env=environment,
        )
        while True:
            try:
                return debugger.wait()
            except KeyboardInterrupt:
                continue
    finally:
        _terminate_process_group(proxy)
        subprocess.run(
            ["wineserver", "-k"],
            env=environment,
            check=False,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )


if __name__ == "__main__":
    raise SystemExit(main())
