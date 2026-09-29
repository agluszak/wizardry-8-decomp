"""Function selection for reccmp's comparison, plus catalog-backed lookups.

reccmp owns comparison: `reccmp-reccmp` decompiles the selected functions and
their originals with Ghidra and diffs them with Ghidriff. This module selects
functions, runs it, and reads its summary.
"""

from __future__ import annotations

import json
import logging
import os
import sys
from collections import Counter
from collections.abc import Iterable
from pathlib import Path
from typing import Any

from reccmp.compare import Compare
from reccmp.compare.vtables import SlotStatus, compare_vtable
from reccmp.project.detect import RecCmpProject, RecCmpTarget
from reccmp.source import SourceIndexError
from reccmp.types import ImageId

from .config import Settings
from .paths import atomic_json, atomic_write, sha256_file
from .subprocesses import CommandFailure, resolve_executable, run

LOGGER = logging.getLogger(__name__)
_PRODUCT_INPUT_SUFFIXES = frozenset(
    {".c", ".cc", ".cpp", ".cxx", ".h", ".hpp", ".hxx", ".inc", ".rc", ".def", ".asm"}
)


def parse_address(value: str) -> int:
    try:
        address = int(value, 16)
    except ValueError as error:
        raise ValueError(f"not a hexadecimal address: {value}") from error
    if address < 0:
        raise ValueError(f"address must not be negative: {value}")
    return address


def addresses_from_files(
    repository: Path,
    target: str,
    paths: Iterable[Path],
    *,
    include_templates: bool = False,
) -> list[int]:
    """Select compiler-bound functions, plus template emissions when requested."""

    from .source_index import load_source_index

    selected = {
        str((path if path.is_absolute() else repository / path).resolve()) for path in paths
    }
    return [
        int(marker["address"])
        for marker in load_source_index(repository)["markers"]
        if (
            marker["marker_kind"] == "FUNCTION"
            or (include_templates and marker["marker_kind"] == "TEMPLATE")
        )
        and marker["target"].upper() == target.upper()
        and str((repository / marker["source_file"]).resolve()) in selected
    ]


def changed_files(repository: Path, since: str | None = None) -> list[Path]:
    """Select changed paths with jj locally and Git in plain CI checkouts."""

    if (repository / ".jj").is_dir() and resolve_executable("jj") is not None:
        command = ["jj", "diff", "--name-only", "--color=never"]
        if since is not None:
            baseline = f"{since[7:]}@origin" if since.startswith("origin/") else since
            command.extend(("--from", baseline))
    else:
        baseline = since
        if baseline is None:
            base_branch = os.environ.get("GITHUB_BASE_REF")
            if base_branch:
                baseline = f"origin/{base_branch}"
            else:
                # Push events expose the previous tip; shallow checkouts often
                # lack HEAD^ so prefer the explicit before SHA when present.
                # Workflow must fetch that SHA — fetch-depth: 2 alone does not
                # guarantee github.event.before for multi-commit pushes.
                before = os.environ.get("GITHUB_EVENT_BEFORE", "").strip()
                baseline = before if before and set(before) != {"0"} else "HEAD^"
        elif baseline.endswith("@origin"):
            baseline = f"origin/{baseline.removesuffix('@origin')}"
        command = ["git", "diff", "--name-only", "--no-renames", baseline]
    result = run(command, cwd=repository)
    return [repository / name for name in result.stdout.splitlines() if name]


def changed_source_files(repository: Path, since: str | None = None) -> list[Path]:
    """Select current changed C/C++ files, including marked headers."""

    return [
        path
        for path in changed_files(repository, since)
        if path.suffix.lower() in {".c", ".cpp", ".cc", ".cxx", ".h", ".hpp", ".hxx"}
        and path.is_file()
    ]


def selected_addresses(
    repository: Path,
    target: str,
    raw: Iterable[str],
    paths: Iterable[Path],
    *,
    include_templates: bool = False,
) -> list[int]:
    selected = set(_resolve_source_selectors(repository, target, raw)) if raw else set()
    paths = list(paths)
    if paths:
        selected.update(
            addresses_from_files(repository, target, paths, include_templates=include_templates)
        )
    if not selected:
        raise ValueError("pass one or more addresses and/or --file source paths")
    return sorted(selected)


def header_dependent_files(settings: Settings, target: str, changed: Iterable[Path]) -> list[Path]:
    """Select header consumers from the existing compiler-backed source index."""
    from .source_index import indexed_targets, load_source_index

    repository = settings.repo_dir.resolve()
    headers = {
        path.resolve().relative_to(repository).as_posix()
        for path in changed
        if path.suffix.lower() in {".h", ".hpp", ".hxx"}
    }
    if not headers:
        return []
    index = load_source_index(repository)
    dependencies = index.get("translation_unit_dependencies")
    if not isinstance(dependencies, list):
        raise SourceIndexError(
            "source index lacks header dependency metadata; run `uv run wiz8 check`"
        )
    roots = indexed_targets(repository)[target.upper()]
    marker_files = {
        marker["source_file"]
        for marker in index["markers"]
        if marker["target"].upper() == target.upper()
        and marker["marker_kind"] in {"FUNCTION", "TEMPLATE"}
    }
    affected: set[str] = set()
    for unit in dependencies:
        source = str(unit.get("source_file") or "")
        if not any(source == root or source.startswith(root.rstrip("/") + "/") for root in roots):
            continue
        file_dependencies = {str(path) for path in unit.get("file_dependencies", [])}
        if headers & file_dependencies:
            affected.add(source)
            affected.update(file_dependencies & marker_files)
    return [repository / path for path in sorted(affected - headers)]


def _numeric_range(value: str) -> tuple[int, int] | None:
    start_text, separator, end_text = value.strip().partition(":")
    try:
        start = int(start_text, 16)
        end = int(end_text, 16) if separator else start
    except ValueError:
        return None
    if start < 0 or end < start:
        raise ValueError(f"invalid function selector range: {value}")
    return start, end


def selectors_require_source_index(values: Iterable[str]) -> bool:
    """Return whether any selector needs semantic source metadata."""

    for value in values:
        numeric = _numeric_range(value)
        if numeric is None or numeric[0] != numeric[1]:
            return True
    return False


def _resolve_source_selectors(repository: Path, target: str, values: Iterable[str]) -> list[int]:
    """Resolve addresses, ranges, and exact source-owned identities for compare."""

    selected: set[int] = set()
    model = None
    by_name = None

    def source_model():
        nonlocal model, by_name
        if model is None:
            from .source_index import source_functions

            model = source_functions(repository, target)
            by_name = {}
            for address, function in model.items():
                by_name.setdefault(function.name, []).append(address)
        return model, by_name

    for value in values:
        numeric = _numeric_range(value)
        if numeric is not None:
            start, end = numeric
            if start == end:
                selected.add(start)
            else:
                model, _ = source_model()
                matches = [address for address in model if start <= address <= end]
                if not matches:
                    raise ValueError(f"no source-owned functions in selector range {value}")
                selected.update(matches)
            continue
        _, by_name = source_model()
        assert by_name is not None
        matches = by_name.get(value, [])
        if not matches:
            # Ghidra's stable default names encode the reviewed entry directly.
            folded = value.casefold()
            for prefix in ("function", "fun_"):
                if folded.startswith(prefix):
                    suffix = value[len(prefix) :]
                    try:
                        selected.add(int(suffix, 16))
                        break
                    except ValueError:
                        pass
            else:
                raise ValueError(f"unknown function selector: {value}")
            continue
        if len(matches) > 1:
            candidates = ", ".join(f"0x{address:08x}" for address in matches[:8])
            raise ValueError(f"ambiguous function selector {value!r}; candidates: {candidates}")
        selected.add(matches[0])
    if not selected:
        raise ValueError("pass one or more function selectors")
    return sorted(selected)


def _project(repository: Path) -> RecCmpProject:
    return RecCmpProject.from_directory(repository / "build" / "decomp")


def comparison_target(repository: Path, target: str) -> RecCmpTarget:
    """Load a configured target and require both comparison products."""

    from .source_index import project_targets

    filename = Path(project_targets(repository)[target.upper()]["filename"])
    expected = (
        repository / "build/decomp" / filename,
        repository / "build/decomp" / filename.with_suffix(".pdb"),
    )
    if any(not path.is_file() for path in expected):
        raise FileNotFoundError("comparison build artifacts are missing; run `uv run wiz8 build`")
    recmp_target = _project(repository).get(target)
    missing = [
        path
        for path in (recmp_target.recompiled_path, recmp_target.recompiled_pdb)
        if path is None or not Path(path).is_file()
    ]
    if missing:
        raise FileNotFoundError("comparison build artifacts are missing; run `uv run wiz8 build`")
    return recmp_target


def warn_if_build_may_be_stale(repository: Path, target: str, recmp_target: RecCmpTarget) -> None:
    """Cheaply compare product mtimes with checked-in product inputs."""

    from .source_index import project_targets

    artifacts = (Path(recmp_target.recompiled_path), Path(recmp_target.recompiled_pdb))
    built_at = min(path.stat().st_mtime_ns for path in artifacts)
    config = project_targets(repository)[target.upper()]
    candidates = [repository / "CMakeLists.txt", repository / "reccmp-project.yml"]
    candidates.extend((repository / "cmake").rglob("*.cmake"))
    candidates.extend(repository.glob("src/*/CMakeLists.txt"))
    candidates.extend(repository.glob("src/*/sources.cmake"))
    roots = config.get("source-root", ())
    if isinstance(roots, str):
        roots = (roots,)
    candidates.extend(
        path
        for root in roots
        for path in (repository / root).rglob("*")
        if path.suffix.lower() in _PRODUCT_INPUT_SUFFIXES
    )
    newer = sorted(
        path.relative_to(repository).as_posix()
        for path in candidates
        if path.is_file() and path.stat().st_mtime_ns > built_at
    )
    if newer:
        LOGGER.warning(
            "comparison build may be stale; relevant inputs are newer than the current build\n"
            "         newer input: %s%s\n"
            "         run `uv run wiz8 build` for fresh comparison results",
            newer[0],
            f" (+{len(newer) - 1} more)" if len(newer) > 1 else "",
        )


# reccmp's Ghidra projects: the original binary's analysis is reused across
# recompiled builds.
GHIDRA_PROJECTS = Path("build/reccmp-ghidra")
_OUTCOMES = ("differences", "no-differences", "unpaired", "analysis-failed")


def report_directory(repository: Path, target: str) -> Path:
    return repository / "build" / "reports" / "compare" / target.lower()


def _run_reccmp(
    repository: Path,
    target: str,
    addresses: list[int],
    ghidra_install_dir: Path,
    *,
    side_by_side: bool,
) -> tuple[dict[str, Any], dict[str, Any] | None]:
    """Run `reccmp-reccmp` for the selected original addresses.

    Returns reccmp's manifest and its summary; the summary is None when no
    selected address is a function reccmp knows."""
    output = report_directory(repository, target)
    for stale in ("manifest.json", "summary.json"):
        (output / stale).unlink(missing_ok=True)
    for stale in output.glob("*.diff"):
        stale.unlink()
    argv: list[str | Path] = [
        sys.executable,
        "-m",
        "reccmp.tools.compare",
        "--target",
        target,
        "--output",
        output,
        "--ghidra-projects",
        repository / GHIDRA_PROJECTS,
    ]
    for address in addresses:
        argv.extend(("--orig-address", f"{address:x}"))
    if side_by_side:
        argv.append("--sxs")
    result = run(
        argv,
        cwd=repository / "build" / "decomp",
        env={**os.environ, "GHIDRA_INSTALL_DIR": str(ghidra_install_dir)},
        log_path=repository / "build" / "logs" / "reccmp-compare.json",
        check=False,
    )
    manifest_path = output / "manifest.json"
    manifest = json.loads(manifest_path.read_text()) if manifest_path.is_file() else None
    summary_path = output / "summary.json"
    if summary_path.is_file():
        assert manifest is not None
        return manifest, json.loads(summary_path.read_text())
    if manifest is not None and not manifest["functions"]:
        return manifest, None
    raise CommandFailure(
        result,
        [line for line in result.stderr.splitlines()[-20:] if line],
        repository / "build" / "logs" / "reccmp-compare.json",
    )


def _function_row(repository: Path, target: str, row: dict[str, Any]) -> dict[str, Any]:
    """reccmp's result for one function, with its code diff moved to a file."""
    result = {key: value for key, value in row.items() if key != "code_diff"}
    diff = row["code_diff"]
    if diff:
        path = report_directory(repository, target) / f"{int(row['orig'], 16):08x}.diff"
        atomic_write(path, "".join(diff))
        result["code_diff"] = {
            "lines": sum(
                1 for line in diff if line[:1] in "+-" and not line.startswith(("+++", "---"))
            ),
            "artifact": str(path.relative_to(repository)),
        }
    return result


def compare_selected(
    repository: Path,
    target: str,
    addresses: list[int],
    ghidra_install_dir: Path,
    *,
    side_by_side: bool = False,
    classify_header_emissions: bool = False,
    classify_template_emissions: bool = False,
) -> dict[str, Any]:
    """Compare the selected functions with reccmp and summarize its results.

    Differences are review material, not failures. The selection fails when a
    comparison did not complete, or a selected authored function has no counterpart.
    Marker-only template non-emissions remain visible without failing the selection."""
    recmp_target = comparison_target(repository, target)
    warn_if_build_may_be_stale(repository, target, recmp_target)
    _manifest, summary = _run_reccmp(
        repository, target, addresses, ghidra_install_dir, side_by_side=side_by_side
    )
    rows = {int(row["orig"], 16): row for row in (summary or {}).get("functions", [])}

    header_emissions: dict[int, Any] = {}
    template_emissions: set[int] = set()
    unlinked_addresses = {
        address
        for address in addresses
        if address not in rows or rows[address]["outcome"] == "unpaired"
    }
    if classify_header_emissions and unlinked_addresses:
        from .source_index import load_source_index, source_functions

        model = source_functions(repository, target)
        named_definitions: set[tuple[str, str]] | None = None

        def is_header_definition(address: int) -> bool:
            nonlocal named_definitions
            marker = model[address]
            if Path(marker.source_file).suffix.casefold() not in {".h", ".hpp", ".hxx", ".inl"}:
                return False
            declaration = marker.declaration
            if declaration is not None:
                return declaration.is_definition
            if marker.marker_name is None:
                return False
            if named_definitions is None:
                named_definitions = {
                    (row["semantic_id"], row["source_file"])
                    for row in load_source_index(repository)["declarations"]
                    if row["target"] == target.upper() and row["is_definition"]
                }
            return (marker.marker_name, marker.source_file) in named_definitions

        header_emissions = {
            address: model[address]
            for address in unlinked_addresses & model.keys()
            if is_header_definition(address)
        }
    if classify_template_emissions and unlinked_addresses:
        from .source_index import load_source_index

        template_addresses = {
            int(marker["address"])
            for marker in load_source_index(repository)["markers"]
            if marker["target"].upper() == target.upper() and marker["marker_kind"] == "TEMPLATE"
        }
        template_emissions = {
            address
            for address in unlinked_addresses & template_addresses
            if (row := rows.get(address)) is not None
            and row["outcome"] == "unpaired"
            and row["recomp"] is None
        }
    functions: list[dict[str, Any]] = []
    for address in sorted(set(addresses)):
        row = rows.get(address)
        if address in header_emissions:
            marker = header_emissions[address]
            functions.append(
                {
                    "orig": f"0x{address:08x}",
                    "name": marker.name,
                    "outcome": "header-emission",
                    "reason": "inline header body has no paired rebuild emission",
                    "source_file": marker.source_file,
                }
            )
        elif address in template_emissions:
            assert row is not None
            emission = _function_row(repository, target, row)
            emission["outcome"] = "template-non-emission"
            emission["reason"] = "retail template emission has no paired rebuild emission"
            functions.append(emission)
        elif row is not None:
            functions.append(_function_row(repository, target, row))
        else:
            functions.append({"orig": f"0x{address:08x}", "outcome": "missing"})

    counts = Counter(row["outcome"] for row in functions)
    output = report_directory(repository, target)
    return {
        "ok": counts["analysis-failed"] == 0 and counts["unpaired"] == 0 and counts["missing"] == 0,
        "selected": len(functions),
        "counts": {
            outcome: counts[outcome]
            for outcome in (*_OUTCOMES, "header-emission", "template-non-emission", "missing")
        },
        "report": {
            "summary": str((output / "summary.json").relative_to(repository))
            if summary is not None
            else None,
            "ghidriff": str((output / f"{target}.ghidriff.md").relative_to(repository))
            if summary is not None
            else None,
        },
        "functions": functions,
    }


def last_comparison(repository: Path, target: str, recompiled: Path) -> dict[str, Any] | None:
    """Counts from the last reccmp report for this target, if it compared the
    current recompiled binary. Never runs a comparison."""
    path = report_directory(repository, target) / "summary.json"
    if not path.is_file():
        return None
    summary = json.loads(path.read_text())
    if summary["inputs"]["recomp"]["sha256"] != sha256_file(recompiled):
        return None
    return {"requested": summary["requested"], **summary["counts"]}


def translate_addresses(repository: Path, target: str, queries: list[int]) -> dict[str, Any]:
    """Look addresses up in reccmp's catalog, from either image."""
    recmp_target = comparison_target(repository, target)
    warn_if_build_may_be_stale(repository, target, recmp_target)
    catalog = Compare.from_target(recmp_target)
    translations: list[dict[str, Any]] = []
    for query in queries:
        direction = "original-to-recompiled"
        match = catalog.get_match(query)
        if match is None:
            direction = "recompiled-to-original"
            entity = catalog.get(ImageId.RECOMP, query)
            orig_addr = entity.orig_addr if entity is not None else None
            match = catalog.get_match(orig_addr) if orig_addr is not None else None
        if match is None:
            translations.append({"query": f"0x{query:08x}", "status": "missing"})
            continue
        basis = catalog.pair_basis(match.orig_addr)
        translations.append(
            {
                "query": f"0x{query:08x}",
                "direction": direction,
                "original": f"0x{match.orig_addr:08x}",
                "recompiled": f"0x{match.recomp_addr:08x}",
                "name": match.best_name(),
                "basis": basis.value if basis is not None else None,
            }
        )
    return {"translations": translations}


def _slot_text(entity: Any, raw: int | None) -> str | None:
    if entity is not None:
        return entity.best_name()
    return f"0x{raw:08x}" if raw is not None else None


def compare_vtables(repository: Path, target: str, class_filter: str | None) -> dict[str, Any]:
    recmp_target = comparison_target(repository, target)
    warn_if_build_may_be_stale(repository, target, recmp_target)
    catalog = Compare.from_target(recmp_target)
    name_filter = class_filter.casefold() if class_filter else None
    rows = []
    for vtable in catalog.get_vtables():
        if name_filter is not None and name_filter not in (vtable.name or "").casefold():
            continue
        comparison = compare_vtable(catalog.db, catalog.orig_bin, catalog.recomp_bin, vtable)
        statuses = {slot.status for slot in comparison.slots}
        status = (
            "match"
            if statuses <= {SlotStatus.MATCH}
            else "different"
            if SlotStatus.DIFFERENT in statuses
            else "unpaired"
            if SlotStatus.UNPAIRED in statuses
            else "code-equivalent"
            if SlotStatus.CODE_EQUIVALENT in statuses
            else "unpaired"
        )
        rows.append(
            {
                "name": vtable.name,
                "original": f"0x{vtable.orig_addr:08x}",
                "recompiled": f"0x{vtable.recomp_addr:08x}",
                "status": status,
                "slots": [
                    {
                        "offset": slot.offset,
                        "status": slot.status.value,
                        "original": _slot_text(slot.orig, slot.orig_raw),
                        "recompiled": _slot_text(slot.recomp, slot.recomp_raw),
                    }
                    for slot in comparison.slots
                    if slot.status != SlotStatus.MATCH
                ],
            }
        )
    if not rows:
        qualifier = f" matching {class_filter!r}" if class_filter else ""
        raise RuntimeError(f"reccmp found zero vtables{qualifier}; refusing vacuous success")
    counts = Counter(row["status"] for row in rows)
    return {
        "ok": counts["different"] == 0 and counts["unpaired"] == 0,
        "count": len(rows),
        "match_count": counts["match"],
        "code_equivalent_count": counts["code-equivalent"],
        "code_equivalent_slot_count": sum(
            slot["status"] == SlotStatus.CODE_EQUIVALENT.value
            for row in rows
            for slot in row["slots"]
        ),
        # A slot at a paired function other than retail's.
        "different_count": counts["different"],
        # Only slots at functions reccmp has not paired, typically bodies the
        # retail link folded (ICF); the comparison build links /OPT:NOICF.
        "unpaired_count": counts["unpaired"],
        "filter": class_filter,
        "vtables": [row for row in rows if row["status"] not in ("match", "code-equivalent")],
    }


def compare_data(repository: Path, target: str) -> dict[str, Any]:
    from reccmp.tools.datacmp import do_the_comparison

    recmp_target = comparison_target(repository, target)
    warn_if_build_may_be_stale(repository, target, recmp_target)
    items = list(do_the_comparison(recmp_target))
    problems = []
    artifact_dir = repository / "build" / "reports" / "datacmp"
    for item in items:
        status = item.result.name.casefold()
        if status == "match":
            continue
        differences = [
            {
                "offset": value.offset,
                "name": value.name,
                "original": value.values[0],
                "recompiled": value.values[1],
            }
            for value in item.compared
            if not value.match
        ]
        problem = {
            "name": item.name,
            "original": f"0x{item.orig_addr:08x}",
            "recompiled": f"0x{item.recomp_addr:08x}",
            "status": status,
            "error": item.error,
            "raw_only": item.raw_only,
            "difference_count": len(differences),
            "witness": differences[:8],
        }
        if len(differences) > 8:
            artifact = artifact_dir / f"{item.orig_addr:08x}.json"
            atomic_json(artifact, {**problem, "differences": differences})
            problem["artifact"] = str(artifact.relative_to(repository))
        problems.append(problem)
    return {
        "ok": not problems,
        "count": len(items),
        "issue_count": len(problems),
        "issues": problems,
    }
