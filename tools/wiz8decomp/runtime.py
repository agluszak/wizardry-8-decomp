from __future__ import annotations

import os
import shutil
import subprocess
import time
from pathlib import Path
from typing import Any

from .config import Settings


def _managed_link(source: Path, destination: Path) -> None:
    if destination.is_symlink():
        if destination.resolve() != source.resolve():
            raise RuntimeError(f"runtime symlink points at the wrong source: {destination}")
        return
    if destination.exists():
        raise RuntimeError(f"runtime staging path is not a managed symlink: {destination}")
    destination.symlink_to(source, target_is_directory=source.is_dir())


def stage_runtime(settings: Settings) -> dict[str, Any]:
    source = settings.work_dir / "variants" / "gog-base"
    stage = settings.repo_dir / "build" / "runtime" / "wiz8"
    executable = settings.repo_dir / "build" / "decomp" / "Wiz8Runtime.exe"
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
    shutil.copy2(executable, stage / "Wiz8Runtime.exe")
    return {"stage": str(stage), "links": links, "executable": str(stage / "Wiz8Runtime.exe")}


def run_game(settings: Settings) -> dict[str, Any]:
    if shutil.which("wine") is None or shutil.which("wineserver") is None:
        raise RuntimeError("wine and wineserver are required to run WIZ8_RUNTIME")
    staged = stage_runtime(settings)
    stage = Path(staged["stage"])
    prefix = Path(os.environ.get("WIZ8_WINE_PREFIX", settings.work_dir / "wine" / "wiz8-runtime"))
    prefix.mkdir(parents=True, exist_ok=True)
    environment = {
        **os.environ,
        "WINEPREFIX": str(prefix),
        "WINEDLLOVERRIDES": "winemenubuilder.exe=d",
    }
    desktop = subprocess.Popen(
        ["wine", "explorer", "/desktop=Wizardry8,640x480"], cwd=stage, env=environment
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
            ["wineserver", "-k"], cwd=stage, env=environment, check=False, capture_output=True
        )
        desktop.wait()
    return {**staged, "wine_prefix": str(prefix), "exit_status": 0}
