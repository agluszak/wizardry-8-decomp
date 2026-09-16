from __future__ import annotations

from collections import Counter
from pathlib import Path
from typing import Annotated, Any

import typer
import yaml

from ..config import Settings

app = typer.Typer(help="Recover MSVC vftables and vbtables from PE images.", no_args_is_help=True)


def _canonical_binary(settings: Settings) -> Path:
    config = yaml.safe_load(
        (settings.repo_dir / "config" / "variants.yml").read_text(encoding="utf-8")
    )
    target = config["canonical_matching_target"]
    return settings.work_dir / "variants" / target["variant"] / target["module"]


def _scan(path: Path, *, repo_dir: Path | None = None) -> dict[str, Any]:
    from ..msvc_table_analysis import enrich_msvc_table_report
    from ..msvc_table_guard import sanitize_receiver_provenance
    from ..msvc_table_source import annotate_source_identities
    from ..msvc_tables import scan_msvc_tables

    report = enrich_msvc_table_report(path, scan_msvc_tables(path, repo_dir=repo_dir))
    sanitize_receiver_provenance(path, report)
    if repo_dir is not None:
        annotate_source_identities(report, repo_dir)
    return report


def _mark_match_ambiguity(result: dict[str, Any]) -> dict[str, Any]:
    """Distinguish unique shape matches from many-to-many candidate groups."""

    unique = 0
    ambiguous = 0
    for match in result.get("matches", []):
        counts = Counter(int(row["report"]) for row in match.get("tables", []))
        is_ambiguous = any(count > 1 for count in counts.values())
        match["match_kind"] = "ambiguous-shape" if is_ambiguous else "unique-shape"
        match["per_report_counts"] = dict(sorted(counts.items()))
        if is_ambiguous:
            ambiguous += 1
        else:
            unique += 1
    result["summary"] = {"unique_shape_groups": unique, "ambiguous_shape_groups": ambiguous}
    return result


@app.command("scan")
def scan_command(
    binary: Annotated[
        Path | None,
        typer.Option(
            "--binary",
            help=(
                "PE image to inspect; defaults to the canonical materialized Wiz8.exe. "
                "If the image exports MSVC ??_7/??_8 symbols they are used as an independent "
                "positive-label recall oracle."
            ),
        ),
    ] = None,
    check: Annotated[
        bool,
        typer.Option(
            "--check",
            help=(
                "Fail if a structural table lacks install evidence, an exported MSVC table is "
                "missed, or a reviewed WIZ8 VTABLE marker is missing from the census."
            ),
        ),
    ] = False,
) -> None:
    """Census tables, receiver provenance, construction families, and slot identities."""

    from .. import command_support as cli

    settings = cli.settings()
    path = binary if binary is not None else _canonical_binary(settings)
    if not path.is_file():
        raise ValueError(f"PE image does not exist: {path}")

    # Checked-in VTABLE/source-index addresses describe the canonical matching
    # image. A custom binary may have shifted addresses, so only the binary's own
    # export table is used as an address oracle for --binary scans.
    report = _scan(path, repo_dir=settings.repo_dir if binary is None else None)
    cli.emit(report)
    validation = report.get("validation")
    if check and isinstance(validation, dict) and validation.get("status") != "passed":
        raise typer.Exit(code=1)


@app.command("compare")
def compare_command(
    binaries: Annotated[
        list[Path],
        typer.Argument(help="Two or more PE builds of the same program/library to compare."),
    ],
) -> None:
    """Match vftables across builds by address-independent slot shape."""

    from .. import command_support as cli
    from ..msvc_table_analysis import compare_table_reports

    if len(binaries) < 2:
        raise ValueError("compare requires at least two PE images")
    for path in binaries:
        if not path.is_file():
            raise ValueError(f"PE image does not exist: {path}")

    reports = [_scan(path) for path in binaries]
    result = _mark_match_ambiguity(compare_table_reports(reports))
    result["binaries"] = [str(path) for path in binaries]
    cli.emit(result)
