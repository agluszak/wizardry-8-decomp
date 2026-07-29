from __future__ import annotations

import logging
from typing import Any

import typer

from . import command_support
from .commands.core import register as register_core
from .commands.core import toolchain_app
from .commands.evidence import app as evidence_app
from .commands.evidence import register_root as register_evidence_root
from .commands.ghidra import app as ghidra_app
from .commands.inputs import app as corpus_app
from .commands.reports import app as report_app
from .commands.sgp import app as sgp_app

app = typer.Typer(
    help="Wizardry 8 reproducible decompilation bootstrap CLI.",
    no_args_is_help=True,
    add_completion=False,
)
app.add_typer(corpus_app, name="corpus")
app.add_typer(ghidra_app, name="ghidra")
app.add_typer(report_app, name="report")
app.add_typer(toolchain_app, name="toolchain")
app.add_typer(evidence_app, name="evidence")
app.add_typer(sgp_app, name="sgp", hidden=True)
register_core(app)
register_evidence_root(app)


class CliState:
    verbose = False
    json_output = False


@app.callback()
def main(
    ctx: typer.Context,
    verbose: bool = typer.Option(False, "--verbose", "-v", help="Enable debug logging."),
    json_output: bool = typer.Option(False, "--json", help="Render command results as JSON."),
) -> None:
    state = CliState()
    state.verbose = verbose
    state.json_output = json_output
    command_support.set_json_output(json_output)
    ctx.obj = state
    logging.basicConfig(
        level=logging.DEBUG if verbose else logging.INFO,
        format="%(levelname)s %(name)s: %(message)s",
    )


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
        settings = command_support.settings()
        checks = []
        version, release = ghidra_version(settings.ghidra_install_dir)
        checks.append(
            {
                "name": "ghidra",
                "ok": version == REQUIRED_GHIDRA_VERSION and release == REQUIRED_GHIDRA_RELEASE,
                "expected": f"{REQUIRED_GHIDRA_VERSION} {REQUIRED_GHIDRA_RELEASE}",
                "actual": f"{version} {release}",
                "path": str(settings.ghidra_install_dir),
            }
        )
        try:
            pyghidra_version = importlib.metadata.version("pyghidra")
        except importlib.metadata.PackageNotFoundError:
            pyghidra_version = None
        checks.append(
            {
                "name": "pyghidra",
                "ok": pyghidra_version == REQUIRED_PYGHIDRA_VERSION,
                "expected": REQUIRED_PYGHIDRA_VERSION,
                "actual": pyghidra_version,
            }
        )
        checks.append(
            {
                "name": "input-directory",
                "ok": settings.input_dir.is_dir() and os.access(settings.input_dir, os.R_OK),
                "path": str(settings.input_dir),
            }
        )
        settings.work_dir.mkdir(parents=True, exist_ok=True)
        try:
            with tempfile.NamedTemporaryFile(prefix="wiz8-doctor-", dir=settings.work_dir):
                work_writable = True
        except OSError:
            work_writable = False
        checks.append(
            {"name": "work-directory", "ok": work_writable, "path": str(settings.work_dir)}
        )
        required = {"7z": ["--help"], "innoextract": ["--version"], "cabextract": ["--version"]}
        optional = {"unshield": ["-V"], "wine": ["--version"], "git-lfs": ["version"]}
        for name, args in required.items():
            info = tool_version(name, args)
            checks.append({"name": name, "ok": bool(info["executable"]), **info, "required": True})
        for name, args in optional.items():
            info = tool_version(name, args)
            checks.append({"name": name, "ok": True, **info, "required": False})
        from .repository import validate_repository_hygiene

        hygiene = validate_repository_hygiene(settings.repo_dir)
        checks.append({"name": "repository-hygiene", **hygiene})
        failed = [item["name"] for item in checks if not item["ok"]]
        if failed:
            raise RuntimeError("doctor failed: " + ", ".join(failed))
        return {"ok": True, "checks": checks, "failures": []}

    command_support.run_action(action)
