from __future__ import annotations

import os
import re
import shutil
import subprocess
import time
from pathlib import Path
from typing import Any

from .config import Settings
from .display import runtime_display


def _managed_link(source: Path, destination: Path) -> None:
    if destination.is_symlink():
        if destination.resolve() != source.resolve():
            raise RuntimeError(f"runtime symlink points at the wrong source: {destination}")
        return
    if destination.exists():
        raise RuntimeError(f"runtime staging path is not a managed symlink: {destination}")
    destination.symlink_to(source, target_is_directory=source.is_dir())


RUNTIME_OBSERVATION = re.compile(r"^WIZ8_RUNTIME_TEST (?P<fields>.+)$")
RUNTIME_SCENARIOS = ("main-menu-startup", "main-menu-exit")


def stage_runtime(settings: Settings, executable_name: str = "Wiz8Runtime.exe") -> dict[str, Any]:
    source = settings.work_dir / "variants" / "gog-base"
    stage = settings.repo_dir / "build" / "runtime" / "wiz8"
    executable = settings.repo_dir / "build" / "decomp" / executable_name
    for name in ("Data", "Dll", "Levels"):
        if not (source / name).is_dir():
            raise RuntimeError(f"missing retail asset directory: {source / name}")
    if not executable.is_file():
        raise RuntimeError(f"runtime executable is not built: {executable}")
    stage.mkdir(parents=True, exist_ok=True)
    (stage / "Saves").mkdir(exist_ok=True)
    links: list[str] = []
    for name in ("Data", "Dll", "Levels", "Patches"):
        candidate = source / name
        if candidate.exists():
            _managed_link(candidate, stage / name)
            links.append(name)
    for candidate in sorted(path for path in source.iterdir() if path.is_file()):
        if candidate.name in {"Wiz8.exe", "Wiz8Runtime.exe", "3DVideo.CFG"}:
            continue
        _managed_link(candidate, stage / candidate.name)
        links.append(candidate.name)
    video_cfg = stage / "3DVideo.CFG"
    if not video_cfg.exists():
        shutil.copy2(settings.repo_dir / "config" / "runtime" / "3DVideo.CFG", video_cfg)
    game_cfg = stage / "Wiz8.CFG"
    if not game_cfg.exists():
        encoded = (settings.repo_dir / "config" / "runtime" / "Wiz8.CFG.hex").read_text()
        game_cfg.write_bytes(bytes.fromhex(encoded))
    shutil.copy2(executable, stage / executable_name)
    return {"stage": str(stage), "links": links, "executable": str(stage / executable_name)}


def _wine_environment(settings: Settings) -> tuple[Path, dict[str, str]]:
    prefix = Path(os.environ.get("WIZ8_WINE_PREFIX", settings.work_dir / "wine" / "wiz8-runtime"))
    prefix.mkdir(parents=True, exist_ok=True)
    return prefix, {
        **os.environ,
        "WINEPREFIX": str(prefix),
        "WINEDLLOVERRIDES": "winemenubuilder.exe=d",
        "WINEDEBUG": "-all",
    }


def _configure_wine_window_management(
    environment: dict[str, str], *, private_display: bool
) -> None:
    """Keep Wine from waiting for a window manager on a private X server."""

    subprocess.run(
        [
            "wine",
            "reg",
            "add",
            r"HKCU\Software\Wine\X11 Driver",
            "/v",
            "Managed",
            "/d",
            "N" if private_display else "Y",
            "/f",
        ],
        env=environment,
        check=True,
        capture_output=True,
        timeout=60,
    )


def _parse_runtime_observation(stdout: str) -> dict[str, str | int]:
    matches = [match for line in stdout.splitlines() if (match := RUNTIME_OBSERVATION.match(line))]
    if len(matches) != 1:
        raise RuntimeError(f"expected one runtime observation, found {len(matches)}")
    fields: dict[str, str | int] = {}
    for item in matches[0].group("fields").split():
        key, separator, value = item.partition("=")
        if not separator:
            raise RuntimeError(f"malformed runtime observation field: {item}")
        fields[key] = int(value) if value.lstrip("-").isdigit() else value
    return fields


def _run_runtime_scenario(
    executable: Path, stage: Path, environment: dict[str, str], scenario: str
) -> dict[str, str | int]:
    try:
        completed = subprocess.run(
            ["wine", f"./{executable.name}", "--scenario", scenario],
            cwd=stage,
            env=environment,
            check=False,
            capture_output=True,
            text=True,
            timeout=45,
        )
    except subprocess.TimeoutExpired as error:
        stdout = (
            error.stdout.decode(errors="replace")
            if isinstance(error.stdout, bytes)
            else error.stdout
        )
        stderr = (
            error.stderr.decode(errors="replace")
            if isinstance(error.stderr, bytes)
            else error.stderr
        )
        raise RuntimeError(
            f"{scenario} timed out inside the recovered runtime: {(stdout or '')}{(stderr or '')}"
        ) from error
    if completed.returncode:
        raise RuntimeError(
            f"{scenario} exited with status {completed.returncode}: "
            f"{completed.stdout}{completed.stderr}"
        )
    observation = _parse_runtime_observation(completed.stdout)
    if observation.get("scenario") != scenario:
        raise RuntimeError(f"runtime observation names the wrong scenario: {observation}")
    return observation


def run_runtime_suite(settings: Settings) -> dict[str, Any]:
    """Run named in-process scenarios in both orders and prove determinism."""

    if shutil.which("wine") is None or shutil.which("wineserver") is None:
        raise RuntimeError("wine and wineserver are required to run WIZ8_RUNTIME_TEST")
    staged = stage_runtime(settings, "Wiz8RuntimeTest.exe")
    stage = Path(staged["stage"])
    executable = Path(staged["executable"])
    prefix, environment = _wine_environment(settings)
    runs: dict[str, dict[str, dict[str, str | int]]] = {}
    with runtime_display(
        environment, default="virtual", log_path=stage / "xvfb-runtime-test.log"
    ) as display:
        _configure_wine_window_management(environment, private_display=display is not None)
        environment["WIZ8_RUNTIME_EXPECTED_SELECTED"] = "2" if display is not None else "0"
        desktop = subprocess.Popen(
            ["wine", "explorer", "/desktop=Wizardry8Tests,640x480"],
            cwd=stage,
            env=environment,
        )
        try:
            time.sleep(1)
            for order_name, scenarios in (
                ("forward", RUNTIME_SCENARIOS),
                ("reverse", tuple(reversed(RUNTIME_SCENARIOS))),
            ):
                runs[order_name] = {
                    scenario: _run_runtime_scenario(executable, stage, environment, scenario)
                    for scenario in scenarios
                }
        finally:
            subprocess.run(
                ["wineserver", "-k"],
                cwd=stage,
                env=environment,
                check=False,
                capture_output=True,
            )
            desktop.wait()
    if runs["forward"] != runs["reverse"]:
        raise RuntimeError("runtime observations depend on scenario order")
    return {
        **staged,
        "wine_prefix": str(prefix),
        "display": display or "host",
        "scenarios": runs["forward"],
        "deterministic": True,
    }


def run_game(settings: Settings) -> dict[str, Any]:
    if shutil.which("wine") is None or shutil.which("wineserver") is None:
        raise RuntimeError("wine and wineserver are required to run WIZ8_RUNTIME")
    staged = stage_runtime(settings)
    stage = Path(staged["stage"])
    prefix, environment = _wine_environment(settings)
    with runtime_display(
        environment, default="host", log_path=stage / "xvfb-runtime.log"
    ) as display:
        _configure_wine_window_management(environment, private_display=display is not None)
        desktop = subprocess.Popen(
            ["wine", "explorer", "/desktop=Wizardry8,640x480"],
            cwd=stage,
            env=environment,
        )
        try:
            time.sleep(1)
            completed = subprocess.run(
                ["wine", "./Wiz8Runtime.exe"], cwd=stage, env=environment, check=False
            )
            if completed.returncode:
                raise RuntimeError(f"Wiz8Runtime.exe exited with status {completed.returncode}")
        finally:
            subprocess.run(
                ["wineserver", "-k"],
                cwd=stage,
                env=environment,
                check=False,
                capture_output=True,
            )
            desktop.wait()
    return {
        **staged,
        "wine_prefix": str(prefix),
        "display": display or "host",
        "exit_status": 0,
    }
