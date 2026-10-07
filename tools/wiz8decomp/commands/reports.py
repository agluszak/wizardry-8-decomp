from __future__ import annotations

import re
from pathlib import Path
from typing import Annotated

import typer

from .. import command_support as cli

app = typer.Typer(help="Generate reports from collected evidence.", no_args_is_help=True)

_INCLUDE = re.compile(r"^\\s*#\\s*include\\s*[<\"]([^>\"]+)[>\"]", re.MULTILINE)


def _includes_directly(source: Path, header: str) -> bool:
    """Whether `source` names repository header `header` in an #include line.

    The dependents come from the base source index, so a source this change
    deletes is gone from the head checkout and includes nothing there."""
    if not source.is_file():
        return False
    text = source.read_text(encoding="utf-8", errors="replace")
    return any(
        header.endswith("/" + included.replace("\\", "/").lstrip("./"))
        for included in _INCLUDE.findall(text)
    )


@app.command("pr-comparison")
def pr_comparison_command(
    target: Annotated[str, typer.Option("--target")],
    head_summary: Annotated[Path | None, typer.Option("--head-summary")] = None,
    base_summary: Annotated[Path | None, typer.Option("--base-summary")] = None,
    head_datacmp: Annotated[Path | None, typer.Option("--head-datacmp")] = None,
    base_datacmp: Annotated[Path | None, typer.Option("--base-datacmp")] = None,
    head_exports: Annotated[Path | None, typer.Option("--head-exports")] = None,
    base_exports: Annotated[Path | None, typer.Option("--base-exports")] = None,
    since: Annotated[
        str | None,
        typer.Option(
            "--since",
            help="Merge base: group regressions by the changed headers their files include directly.",
        ),
    ] = None,
) -> None:
    """Summarize PR-head comparison health and its change from the merge base."""

    from ..reports.comparison_delta import pr_comparison_report

    header_includers = None
    if since is not None:
        from ..comparison import changed_files, header_dependent_files

        settings = cli.settings()
        repository = settings.repo_dir.resolve()
        header_includers = {}
        for path in changed_files(repository, since):
            if path.suffix.lower() not in {".h", ".hpp", ".hxx"} or not path.is_file():
                continue
            header = path.resolve().relative_to(repository).as_posix()
            dependents = header_dependent_files(settings, target, [path])
            header_includers[header] = {str(path.resolve())} | {
                str(file.resolve()) for file in dependents if _includes_directly(file, header)
            }

    result = pr_comparison_report(
        target,
        head_summary_path=head_summary,
        base_summary_path=base_summary,
        head_datacmp_path=head_datacmp,
        base_datacmp_path=base_datacmp,
        header_includers=header_includers,
        head_exports_path=head_exports,
        base_exports_path=base_exports,
    )
    cli.emit(result)
    if not result["ok"]:
        raise typer.Exit(1)
