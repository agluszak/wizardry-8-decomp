"""Run function recovery as one read-only Ghidra headless script."""

from __future__ import annotations

import json
import tempfile
from pathlib import Path
from typing import Any

from .. import subprocesses
from ..config import Settings
from ..paths import atomic_write


def parse_selection(text: str) -> tuple[int, int | None]:
    """Parse ``0xADDR`` or an inclusive ``0xSTART:0xEND`` entry range."""

    raw = text.strip()
    if not raw:
        raise ValueError("empty selection")
    start_text, separator, end_text = raw.partition(":")
    try:
        start = int(start_text, 0)
        end = int(end_text, 0) if separator else None
    except ValueError as error:
        raise ValueError(f"invalid selection {text!r}: {error}") from error
    if start < 0 or (end is not None and end < 0):
        raise ValueError(f"invalid selection {text!r}: addresses must be non-negative")
    if end is not None and end < start:
        raise ValueError(f"invalid selection {text!r}: range end precedes start")
    return start, end


def _program_name(settings: Settings, selector: str) -> str:
    from .workspace import check_project_owner, seed_record

    check_project_owner(settings)
    record = seed_record(settings, selector)
    project = settings.project_dir / f"{settings.project_name}.gpr"
    if not project.is_file():
        raise RuntimeError(
            f"reviewed Ghidra project is not restored at {project}; "
            "run `uv run wiz8 ghidra restore` first"
        )
    return str(record["program"])


def run_ghidra_script(
    settings: Settings,
    script: str,
    args: list[str],
    *,
    program_name: str,
) -> Path:
    """Run one source-bundle script read-only and return its JSON output path."""

    script_dir = settings.repo_dir / "tools" / "ghidra-scripts"
    if not (script_dir / script).is_file():
        raise RuntimeError(f"missing Ghidra script {script_dir / script}")
    headless = settings.ghidra_install_dir / "support" / "analyzeHeadless"
    if not headless.is_file():
        raise RuntimeError(f"missing analyzeHeadless at {headless}")
    output_dir = settings.build_dir / "recover"
    output_dir.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(
        prefix="ghidra-", suffix=".json", dir=output_dir, delete=False
    ) as temporary:
        output = Path(temporary.name)
    output.unlink()
    command = [
        headless,
        settings.project_dir,
        settings.project_name,
        "-process",
        program_name,
        "-readOnly",
        "-noanalysis",
        "-scriptPath",
        script_dir,
        "-postScript",
        script,
        *args,
        "--output",
        output,
    ]
    try:
        subprocesses.run(
            command,
            cwd=settings.repo_dir,
            log_path=settings.build_dir / "logs" / f"{Path(script).stem}.json",
        )
    except Exception:
        output.unlink(missing_ok=True)
        raise
    if not output.is_file():
        raise RuntimeError(f"{script} completed without writing {output}")
    return output


def _recover(
    settings: Settings,
    selections: list[str],
    *,
    program_selector: str,
    explain: bool,
) -> dict[str, Any]:
    if not selections:
        raise ValueError("pass at least one function address or range")
    for selection in selections:
        parse_selection(selection)
    program_name = _program_name(settings, program_selector)
    args = ["--source-index", str(settings.build_dir / "source-index.json")]
    if explain:
        args.append("--explain")
    args.extend(selections)
    result_path = run_ghidra_script(settings, "Wiz8Recover.java", args, program_name=program_name)
    try:
        return json.loads(result_path.read_text(encoding="utf-8"))
    finally:
        result_path.unlink(missing_ok=True)


def recover_functions(
    settings: Settings,
    selections: list[str],
    *,
    program_selector: str = "wiz8",
    output: Path | None = None,
) -> dict[str, Any]:
    """Recover one or more selected function definitions."""

    result = _recover(settings, selections, program_selector=program_selector, explain=False)
    if output is not None:
        atomic_write(output, str(result["text"]))
        result["outputs"] = [str(output)]
    return result


def explain_function(
    settings: Settings,
    selection: str,
    *,
    program_selector: str = "wiz8",
) -> dict[str, Any]:
    """Format the structured facts for one recovered function."""

    start, end = parse_selection(selection)
    if end is not None:
        raise ValueError("explain takes one plain address")
    result = _recover(settings, [selection], program_selector=program_selector, explain=True)
    recovery = result["exports"][0]["recovery"]
    lines = [f"0x{start:08x} {recovery['emission_kind']} ({recovery['source_kind']})"]
    lines.extend(
        f"{fact['status']:<12} {fact['pass']}: {fact['detail']}" for fact in recovery["passes"]
    )
    if not recovery["passes"]:
        lines.append("no recognizer applied or declined; verbatim rendering")
    lines.extend(f"defect      {defect}" for defect in recovery["defects"])
    return {
        "program": result["program"],
        "address": f"0x{start:08x}",
        "text": "\n".join(lines) + "\n",
    }
