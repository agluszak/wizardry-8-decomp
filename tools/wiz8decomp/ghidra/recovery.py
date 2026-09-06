"""Run function recovery inside the checkout-scoped Ghidra owner."""

from __future__ import annotations

import json
import tempfile
from pathlib import Path
from typing import Any

from ..config import Settings
from ..paths import atomic_write
from .workspace import resolve_seed_program


def _program_name(settings: Settings, selector: str) -> str:
    # Seed restoration is idempotent and owns the project/checkout guard.  A
    # recovery command should not require agents to manage that lifecycle.
    return resolve_seed_program(settings, selector)


def _recover(
    settings: Settings,
    selections: list[str],
    *,
    program_selector: str,
    explain: bool,
    include_body: bool = False,
) -> dict[str, Any]:
    if not selections:
        raise ValueError("pass at least one function address or range")
    from .env import open_program
    from .query import resolve_function_selectors

    with open_program(settings, program_selector) as program:
        normalized = [
            f"0x{address:08x}" for address in resolve_function_selectors(program, list(selections))
        ]
        args = ["--source-index", str(settings.build_dir / "source-index.json")]
        if explain:
            args.append("--explain")
        if include_body:
            args.append("--include-body")
        args.extend(normalized)
        return _execute_script(settings, program, "Wiz8Recover.java", args)


def _execute_script(
    settings: Settings, program: Any, script_name: str, arguments: list[str]
) -> dict[str, Any]:
    """Compile and execute a repository Java script against an open program."""

    from generic.jar import ResourceFile
    from ghidra.app.script import GhidraScriptUtil, GhidraState, JavaScriptProvider
    from ghidra.util.task import TaskMonitor
    from java.io import PrintWriter, StringWriter

    script_dir = settings.repo_dir / "tools" / "ghidra-scripts"
    script_path = script_dir / script_name
    if not script_path.is_file():
        raise RuntimeError(f"missing Ghidra script {script_path}")
    root = ResourceFile(str(script_dir))
    host = GhidraScriptUtil.getBundleHost()
    acquired_host = host is None
    if host is None:
        host = GhidraScriptUtil.acquireBundleHostReference()
    try:
        if host.getExistingGhidraBundle(root) is None:
            host.add(root, True, False)
        diagnostics = StringWriter()
        script = JavaScriptProvider().getScriptInstance(
            ResourceFile(str(script_path)), PrintWriter(diagnostics)
        )
        output_dir = settings.build_dir / "recover"
        output_dir.mkdir(parents=True, exist_ok=True)
        with tempfile.NamedTemporaryFile(
            prefix="ghidra-", suffix=".json", dir=output_dir, delete=False
        ) as temporary:
            output = Path(temporary.name)
        output.unlink()
        try:
            script.setScriptArgs([*arguments, "--output", str(output)])
            script.execute(
                GhidraState(None, None, program, None, None, None),
                TaskMonitor.DUMMY,
                PrintWriter(diagnostics),
            )
            if not output.is_file():
                raise RuntimeError(
                    f"{script_name} completed without writing {output}: {diagnostics}"
                )
            return json.loads(output.read_text(encoding="utf-8"))
        finally:
            output.unlink(missing_ok=True)
    finally:
        if acquired_host:
            GhidraScriptUtil.releaseBundleHostReference()


def recover_functions(
    settings: Settings,
    selections: list[str],
    *,
    program_selector: str = "wiz8",
    output: Path | None = None,
    include_body: bool = False,
) -> dict[str, Any]:
    """Recover one or more selected function definitions."""

    result = _recover(
        settings,
        selections,
        program_selector=program_selector,
        explain=False,
        include_body=include_body,
    )
    if output is not None:
        generated = [str(item["generated_code"]) for item in result.get("exports", [])]
        atomic_write(output, "\n".join(generated))
        result["outputs"] = [str(output)]
    return result


def explain_functions(
    settings: Settings,
    selections: list[str],
    *,
    program_selector: str = "wiz8",
) -> dict[str, Any]:
    """Format structured recovery facts for addresses, ranges, or mixed selections."""

    from .env import open_program
    from .query import resolve_function_selectors

    with open_program(settings, program_selector) as program:
        normalized = [
            f"0x{address:08x}" for address in resolve_function_selectors(program, selections)
        ]
    result = _recover(settings, normalized, program_selector=program_selector, explain=True)
    functions: list[dict[str, Any]] = []
    for item in result.get("exports", []):
        recovery = item["recovery"]
        entry = str(item["entry"])
        name = item.get("name") or recovery.get("name") or ""
        passes = list(recovery.get("passes", []))
        functions.append(
            {
                "entry": entry,
                "name": name,
                "emission_kind": recovery.get("emission_kind"),
                "source_kind": recovery.get("source_kind"),
                "passes": passes,
                "defects": recovery.get("defects", []),
            }
        )
    return {
        "schema": "wiz8.recovery-explanation",
        "program": result["program"],
        "selections": normalized,
        "functions": functions,
    }


def explain_function(
    settings: Settings, selection: str, *, program_selector: str = "wiz8"
) -> dict[str, Any]:
    return explain_functions(settings, [selection], program_selector=program_selector)
