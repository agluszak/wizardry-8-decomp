from __future__ import annotations

import re
from pathlib import Path
from typing import Annotated

import typer

from .. import command_support as cli

app = typer.Typer(help="Generate reports from collected evidence.", no_args_is_help=True)


@app.command("compare")
def comparison_command(
    addresses: Annotated[
        list[str] | None, typer.Argument(help="Original addresses to inspect.")
    ] = None,
    program: Annotated[str, typer.Option("--program")] = "wiz8",
    report: Annotated[
        Path | None, typer.Option(help="Saved summary.json; defaults to the latest run.")
    ] = None,
    against: Annotated[
        Path | None, typer.Option(help="Earlier summary.json; show changed results only.")
    ] = None,
    files: Annotated[
        list[Path] | None, typer.Option("--file", help="Restrict to these source owners.")
    ] = None,
    outcome: Annotated[str | None, typer.Option(help="Filter by reccmp outcome.")] = None,
    limit: Annotated[int, typer.Option(min=1, max=100, help="Maximum function rows.")] = 20,
    diff_lines: Annotated[
        int, typer.Option(min=0, max=200, help="Total code-diff line budget.")
    ] = 0,
) -> None:
    """Inspect a saved comparison without compiling or starting Ghidra."""
    from ..comparison import parse_address
    from ..reports.comparison import comparison_report
    from ..source_index import target_for_program

    settings = cli.settings()
    cli.emit(
        comparison_report(
            settings.repo_dir,
            target_for_program(settings.repo_dir, program),
            report=report,
            against=against,
            addresses=[parse_address(address) for address in addresses or []],
            files=files,
            outcome=outcome,
            limit=limit,
            diff_lines=diff_lines,
        )
    )


_INCLUDE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.MULTILINE)


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
    head_ghidriff: Annotated[Path | None, typer.Option("--head-ghidriff")] = None,
    base_ghidriff: Annotated[Path | None, typer.Option("--base-ghidriff")] = None,
    head_datacmp: Annotated[Path | None, typer.Option("--head-datacmp")] = None,
    base_datacmp: Annotated[Path | None, typer.Option("--base-datacmp")] = None,
    head_exports: Annotated[Path | None, typer.Option("--head-exports")] = None,
    base_exports: Annotated[Path | None, typer.Option("--base-exports")] = None,
    head_direct_calls: Annotated[Path | None, typer.Option("--head-direct-calls")] = None,
    base_direct_calls: Annotated[Path | None, typer.Option("--base-direct-calls")] = None,
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
        head_ghidriff_path=head_ghidriff,
        base_ghidriff_path=base_ghidriff,
        head_datacmp_path=head_datacmp,
        base_datacmp_path=base_datacmp,
        head_direct_calls_path=head_direct_calls,
        base_direct_calls_path=base_direct_calls,
        header_includers=header_includers,
        head_exports_path=head_exports,
        base_exports_path=base_exports,
    )
    cli.emit(result)
    if not result["ok"]:
        raise typer.Exit(1)


@app.command("semantic-debt")
def semantic_debt_command(
    program: str = typer.Option("wiz8", "--program"),
) -> None:
    """Rank provisional recovery work without turning it into a gate."""

    from ..reports.semantic_debt import semantic_debt_report
    from ..source_index import target_for_program, warn_if_source_index_may_be_stale

    settings = cli.settings()
    target = target_for_program(settings.repo_dir, program)
    warn_if_source_index_may_be_stale(settings.repo_dir, target)
    cli.emit(semantic_debt_report(settings.repo_dir, target))


@app.command("source-oracle")
def source_oracle_command() -> None:
    """Report proven available-source ownership and fail on Wizardry misplaced recoveries."""

    from ..source_index import warn_if_source_index_may_be_stale
    from ..source_oracle import source_oracle_report

    settings = cli.settings()
    warn_if_source_index_may_be_stale(settings.repo_dir, "WIZ8")
    report = source_oracle_report(settings.repo_dir)
    cli.emit(report)
    if report["status"] != "passed":
        raise typer.Exit(code=1)


@app.command("semantic-names")
def semantic_names_command() -> None:
    """Rank frequently referenced FunctionXXXXXXXX declarations for recovery."""

    from ..reports.semantic_debt import semantic_name_opportunity_report
    from ..source_index import warn_if_source_index_may_be_stale

    settings = cli.settings()
    warn_if_source_index_may_be_stale(settings.repo_dir, "WIZ8")
    cli.emit(semantic_name_opportunity_report(settings.repo_dir))


@app.command("merge-preservation")
def merge_preservation_command(
    base: Annotated[str, typer.Option("--base", help="Base revision, e.g. origin/main.")],
    head: str | None = typer.Option(
        None,
        "--head",
        help="Explicit result revision; defaults to the current Jujutsu change or Git working tree.",
    ),
) -> None:
    """Compare FUNCTION/GLOBAL/VTABLE identities by retail address between two revisions."""

    from ..merge_preservation import base_ancestry_report, merge_preservation_report

    repository = cli.settings().repo_dir
    ancestry = base_ancestry_report(repository, base, head)
    if ancestry["status"] != "passed":
        cli.emit({"status": "failed", "base": base, "base_ancestry": ancestry})
        raise typer.Exit(code=1)
    report = merge_preservation_report(
        repository,
        ancestry["base"],
        ancestry["head"] if head is not None else None,
    )
    report["requested_base"] = base
    report["base_ancestry"] = ancestry
    cli.emit(report)
    if report["status"] != "passed":
        raise typer.Exit(code=1)
