from __future__ import annotations

import json
import logging
import subprocess
import sys
from pathlib import Path
from typing import Any

import typer
from rich.console import Console
from rich.json import JSON

from .commands.core import register as register_core
from .commands.core import toolchain_app
from .commands.evidence import app as evidence_app
from .commands.evidence import register_root as register_evidence_root
from .commands.ghidra import app as ghidra_app
from .commands.inputs import app as corpus_app
from .commands.overlay import app as overlay_app
from .commands.reports import app as report_app
from .commands.sgp import app as sgp_app
from .config import load_settings

app = typer.Typer(
    help="Wizardry 8 reproducible decompilation bootstrap CLI.",
    no_args_is_help=True,
    add_completion=False,
)
app.add_typer(corpus_app, name="corpus")
app.add_typer(ghidra_app, name="ghidra")
ghidra_app.add_typer(overlay_app, name="overlay")
app.add_typer(report_app, name="report")
app.add_typer(toolchain_app, name="toolchain")
app.add_typer(evidence_app, name="evidence")
app.add_typer(sgp_app, name="sgp", hidden=True)
register_core(app)
register_evidence_root(app)

console = Console()
logger = logging.getLogger(__name__)
_JSON_OUTPUT = False


class CliState:
    verbose = False
    json_output = False


@app.callback()
def main(
    ctx: typer.Context,
    verbose: bool = typer.Option(False, "--verbose", "-v", help="Enable debug logging."),
    json_output: bool = typer.Option(False, "--json", help="Render command results as JSON."),
) -> None:
    global _JSON_OUTPUT
    state = CliState()
    state.verbose = verbose
    state.json_output = json_output
    _JSON_OUTPUT = json_output
    ctx.obj = state
    logging.basicConfig(
        level=logging.DEBUG if verbose else logging.INFO,
        format="%(levelname)s %(name)s: %(message)s",
    )


def _settings():
    try:
        settings = load_settings()
        assert settings is not None
        return settings
    except Exception as error:
        raise typer.BadParameter(str(error)) from error


def _emit(value: Any, *, force_json: bool = False) -> None:
    if _JSON_OUTPUT or force_json:
        sys.stdout.write(json.dumps(value, indent=2, sort_keys=False, ensure_ascii=False) + "\n")
    elif isinstance(value, (dict, list)):
        console.print(JSON.from_data(value))
    else:
        console.print(value)


def _run_action(action: Any, *, force_json: bool = False) -> None:
    try:
        _emit(action(), force_json=force_json)
    except Exception as error:
        if logger.isEnabledFor(logging.DEBUG):
            logger.exception("command failed")
        console.print(f"[red]error:[/red] {error}", highlight=False)
        raise typer.Exit(1) from error


def _tracked_copyrighted(settings: Any) -> list[str]:
    if not (settings.repo_dir / ".git").exists():
        return []
    completed = subprocess.run(
        ["git", "ls-files", "-z"], cwd=settings.repo_dir, capture_output=True, check=False
    )
    suspect_suffixes = {
        ".exe", ".dll", ".iso", ".zip", ".7z", ".rar", ".cab", ".bik", ".wav",
        ".mp3", ".slf", ".asi", ".m3d",
    }
    suspects = []
    for raw in completed.stdout.split(b"\0"):
        if not raw:
            continue
        relative = raw.decode("utf-8", errors="replace")
        path = settings.repo_dir / relative
        if path.suffix.casefold() in suspect_suffixes and not relative.startswith(
            "vendor/ghidra/exports/"
        ):
            suspects.append(relative)
    return sorted(suspects)


def _reccmp_original(target: str) -> Path | None:
    import yaml

    user_config = _settings().repo_dir / "reccmp-user.yml"
    if not user_config.is_file():
        return None
    configured = yaml.safe_load(user_config.read_text(encoding="utf-8")) or {}
    path = (configured.get("targets") or {}).get(target, {}).get("path")
    if not path:
        return None
    resolved = Path(str(path).strip())
    return resolved if resolved.is_file() else None


def doctor() -> None:
    """Compatibility entry point used by the core command adapter."""
    import importlib.metadata
    import os
    import tempfile

    from .config import (
        REQUIRED_GHIDRA_RELEASE,
        REQUIRED_GHIDRA_VERSION,
        REQUIRED_PYGHIDRA_VERSION,
        ghidra_version,
    )
    from .subprocesses import tool_version

    def action() -> dict[str, Any]:
        settings = _settings()
        checks = []
        version, release = ghidra_version(settings.ghidra_install_dir)
        checks.append({
            "name": "ghidra",
            "ok": version == REQUIRED_GHIDRA_VERSION and release == REQUIRED_GHIDRA_RELEASE,
            "expected": f"{REQUIRED_GHIDRA_VERSION} {REQUIRED_GHIDRA_RELEASE}",
            "actual": f"{version} {release}",
            "path": str(settings.ghidra_install_dir),
        })
        try:
            pyghidra_version = importlib.metadata.version("pyghidra")
        except importlib.metadata.PackageNotFoundError:
            pyghidra_version = None
        checks.append({
            "name": "pyghidra",
            "ok": pyghidra_version == REQUIRED_PYGHIDRA_VERSION,
            "expected": REQUIRED_PYGHIDRA_VERSION,
            "actual": pyghidra_version,
        })
        checks.append({
            "name": "input-directory",
            "ok": settings.input_dir.is_dir() and os.access(settings.input_dir, os.R_OK),
            "path": str(settings.input_dir),
        })
        settings.work_dir.mkdir(parents=True, exist_ok=True)
        try:
            with tempfile.NamedTemporaryFile(prefix="wiz8-doctor-", dir=settings.work_dir):
                work_writable = True
        except OSError:
            work_writable = False
        checks.append({"name": "work-directory", "ok": work_writable, "path": str(settings.work_dir)})
        required = {"7z": ["--help"], "innoextract": ["--version"], "cabextract": ["--version"]}
        optional = {"unshield": ["-V"], "wine": ["--version"], "git-lfs": ["version"]}
        for name, args in required.items():
            info = tool_version(name, args)
            checks.append({"name": name, "ok": bool(info["executable"]), **info, "required": True})
        for name, args in optional.items():
            info = tool_version(name, args)
            checks.append({"name": name, "ok": True, **info, "required": False})
        suspects = _tracked_copyrighted(settings)
        checks.append({"name": "tracked-copyrighted-files", "ok": not suspects, "files": suspects})
        failed = [item["name"] for item in checks if not item["ok"]]
        if failed:
            raise RuntimeError("doctor failed: " + ", ".join(failed))
        return {"ok": True, "checks": checks, "failures": []}

    _run_action(action)
