"""Focused comparison and address-translation workflows over reccmp's API."""

from __future__ import annotations

import logging
import os
from collections.abc import Iterable
from dataclasses import asdict
from pathlib import Path
from typing import Any

from reccmp.compare import Compare
from reccmp.compare.report import ReccmpComparedEntity
from reccmp.project.detect import RecCmpProject, RecCmpTarget
from reccmp.source import SourceIndexError

from .config import Settings
from .paths import atomic_json
from .subprocesses import resolve_executable, run

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


def addresses_from_files(repository: Path, target: str, paths: Iterable[Path]) -> list[int]:
    """Select compiler-bound FUNCTION markers from the shared source index."""

    from .source_index import load_source_index

    selected = {
        str((path if path.is_absolute() else repository / path).resolve()) for path in paths
    }
    return [
        int(marker["address"])
        for marker in load_source_index(repository)["markers"]
        if marker["marker_kind"] == "FUNCTION"
        and marker["target"].upper() == target.upper()
        and str((repository / marker["source_file"]).resolve()) in selected
    ]


def changed_files(repository: Path, since: str | None = None) -> list[Path]:
    """Select changed paths with jj locally and Git in plain CI checkouts."""

    if (repository / ".jj").is_dir() and resolve_executable("jj") is not None:
        command = ["jj", "diff", "--name-only", "--color=never"]
        if since is not None:
            command.extend(("--from", since))
    else:
        baseline = since
        if baseline is None:
            base_branch = os.environ.get("GITHUB_BASE_REF")
            baseline = f"origin/{base_branch}" if base_branch else "HEAD^"
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
    repository: Path, target: str, raw: Iterable[str], paths: Iterable[Path]
) -> list[int]:
    selected = set(_resolve_source_selectors(repository, target, raw)) if raw else set()
    paths = list(paths)
    if paths:
        selected.update(addresses_from_files(repository, target, paths))
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
            "source index lacks header dependency metadata; run `uv run wiz8 analyze source-index`"
        )
    roots = indexed_targets(repository)[target.upper()]
    marker_files = {
        marker["source_file"]
        for marker in index["markers"]
        if marker["target"].upper() == target.upper() and marker["marker_kind"] == "FUNCTION"
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
    if any(path.is_file() and path.stat().st_mtime_ns > built_at for path in candidates):
        LOGGER.warning(
            "comparison build may be stale; relevant inputs are newer than the current build\n"
            "         run `uv run wiz8 build` for fresh comparison results"
        )


def _function_result(entity: ReccmpComparedEntity) -> dict[str, Any]:
    """Convert one reccmp comparison into the external JSON row."""

    analysis = entity.analysis
    result: dict[str, Any] = {
        "address": f"0x{entity.orig_addr:08x}",
        "recompiled": (f"0x{entity.recomp_addr:08x}" if entity.recomp_addr is not None else None),
        "name": entity.name,
        "raw_matching": entity.accuracy,
        "effective_matching": entity.effective_accuracy,
        "status": analysis.status.value,
    }
    if analysis.effective_reasons:
        result["effective_reasons"] = list(analysis.effective_reasons)
    if analysis.difference is not None:
        result["difference"] = asdict(analysis.difference)
    if analysis.inconclusive_reason is not None:
        result["reason"] = analysis.inconclusive_reason
    if analysis.inconclusive_location is not None:
        result["location"] = asdict(analysis.inconclusive_location)
    return result


def compare_selected(
    repository: Path,
    target: str,
    addresses: list[int],
    *,
    include_windows: bool = True,
    classify_header_emissions: bool = False,
) -> dict[str, Any]:
    recmp_target = comparison_target(repository, target)
    warn_if_build_may_be_stale(repository, target, recmp_target)
    pairing_logger = logging.getLogger("reccmp.compare.lines")
    previous_level = pairing_logger.level
    # Selected comparison reports unresolved requested addresses itself. The
    # reccmp pairing pass also logs every unrelated unlinked inline body while
    # constructing the engine, which made the known header emissions look like
    # failures even when they were not selected.
    pairing_logger.setLevel(logging.CRITICAL)
    try:
        engine = Compare.from_target(recmp_target, orig_addrs=addresses)
        matches = {
            entity.orig_addr: entity
            for entity in engine.compare_addresses(
                orig_addrs=addresses,
                include_diff=False,
                include_exact_diff=False,
            )
        }
    finally:
        pairing_logger.setLevel(previous_level)
    window_images: dict[str, Any] = {}
    header_emissions: dict[int, Any] = {}
    missing_addresses = set(addresses) - matches.keys()
    if classify_header_emissions and missing_addresses:
        from .source_index import source_functions

        model = source_functions(repository, target)

        def is_header_definition(address: int) -> bool:
            marker = model[address]
            declaration = marker.declaration
            return (
                Path(marker.source_file).suffix.casefold() in {".h", ".hpp", ".hxx", ".inl"}
                and declaration is not None
                and declaration.is_definition
            )

        header_emissions = {
            address: model[address]
            for address in missing_addresses & model.keys()
            if is_header_definition(address)
        }
    functions: list[dict[str, Any]] = []
    for address in sorted(set(addresses)):
        entity = matches.get(address)
        if entity is None:
            if address in header_emissions:
                marker = header_emissions[address]
                functions.append(
                    {
                        "address": f"0x{address:08x}",
                        "name": marker.name,
                        "status": "header-emission",
                        "reason": "inline header body has no standalone linked symbol",
                        "source_file": marker.source_file,
                    }
                )
                continue
            functions.append({"address": f"0x{address:08x}", "status": "missing"})
            continue
        row = _function_result(entity)
        if include_windows and row["status"] == "mismatch":
            window = _instruction_windows(recmp_target, entity, images=window_images)
            if window:
                row["instruction_window"] = window
                difference = row.get("difference") or {}
                if difference.get("kind") == "branch_target" and not _paired_branch_witness(window):
                    row["reported_difference"] = difference
                    row["difference"] = {"kind": "alignment_or_structure"}
        functions.append(row)

    exact = sum(row["status"] == "exact" for row in functions)
    effective = sum(row["status"] == "effective" for row in functions)
    missing = sum(row["status"] == "missing" for row in functions)
    emitted = sum(row["status"] == "header-emission" for row in functions)
    return {
        "ok": exact + effective + emitted == len(functions),
        "selected": len(functions),
        "exact": exact,
        "effective": effective,
        "below_exact": len(functions) - exact - effective - missing - emitted,
        "missing": missing,
        "header_emissions": emitted,
        "functions": functions,
    }


def _paired_branch_witness(window: dict[str, list[dict[str, Any]]]) -> bool:
    """A branch-target diagnosis requires corresponding branch instructions on both sides."""

    def divergent_mnemonic(side: str) -> str:
        row = next((item for item in window.get(side, []) if item.get("divergence")), None)
        return str(row.get("instruction", "")).split(maxsplit=1)[0].casefold() if row else ""

    mnemonics = (divergent_mnemonic("original"), divergent_mnemonic("recomp"))
    return all(value.startswith("j") and value != "jmp" for value in mnemonics)


def _instruction_windows(
    target: RecCmpTarget,
    entity: ReccmpComparedEntity,
    *,
    radius: int = 3,
    images: dict[str, Any] | None = None,
) -> dict[str, list[dict[str, Any]]]:
    """Decode a bounded window around reccmp's structured first divergence."""

    from .binary.code import disassembler
    from .binary.image import PeImage

    difference = entity.analysis.difference
    if difference is None:
        return {}
    paths = {"original": target.original_path, "recomp": target.recompiled_path}
    starts: dict[str, int | None] = {
        "original": entity.orig_addr,
        "recomp": entity.recomp_addr,
    }
    sides = {"original": difference.orig, "recomp": difference.recomp}
    result: dict[str, list[dict[str, Any]]] = {}
    for side in ("original", "recomp"):
        start = starts[side]
        index = sides[side].instruction_index
        if start is None or not isinstance(index, int):
            continue
        try:
            if images is None:
                image = PeImage(paths[side])
            else:
                image = images.get(side)
                if image is None:
                    image = PeImage(paths[side])
                    images[side] = image
            instructions = list(disassembler().disasm(image.read(start, 0x4000), start))
        except (OSError, ValueError):
            continue
        low, high = max(0, index - radius), min(len(instructions), index + radius + 1)
        result[side] = [
            {
                "address": f"0x{instruction.address:08x}",
                "instruction": f"{instruction.mnemonic} {instruction.op_str}".rstrip(),
                "divergence": position == index,
            }
            for position, instruction in enumerate(instructions[low:high], start=low)
        ]
    return result


def translate_addresses(repository: Path, target: str, queries: list[int]) -> dict[str, Any]:
    recmp_target = comparison_target(repository, target)
    warn_if_build_may_be_stale(repository, target, recmp_target)
    engine = Compare.from_target(recmp_target)
    entities = list(
        engine.compare_addresses(
            orig_addrs=queries,
            recomp_addrs=queries,
            include_diff=False,
            include_exact_diff=False,
        )
    )
    by_original = {entity.orig_addr: entity for entity in entities}
    by_recompiled = {
        entity.recomp_addr: entity for entity in entities if entity.recomp_addr is not None
    }
    translations: list[dict[str, Any]] = []
    for query in queries:
        entity = by_original.get(query)
        direction = "original-to-recompiled"
        if entity is None:
            entity = by_recompiled.get(query)
            direction = "recompiled-to-original"
        if entity is None:
            translations.append({"query": f"0x{query:08x}", "status": "missing"})
            continue
        translations.append(
            {
                "query": f"0x{query:08x}",
                "direction": direction,
                "original": f"0x{entity.orig_addr:08x}",
                "recompiled": (
                    f"0x{entity.recomp_addr:08x}" if entity.recomp_addr is not None else None
                ),
                "name": entity.name,
                "raw_matching": entity.accuracy,
                "status": entity.analysis.status.value,
            }
        )
    return {"translations": translations}


def compare_linked_image(repository: Path, target: str) -> dict[str, Any]:
    """Run the whole linked-image comparison through reccmp's report API."""

    from reccmp.compare.report import report_function_accuracy

    recmp_target = _project(repository).get(target)
    engine = Compare.from_target(recmp_target)
    report = engine.to_report(
        recmp_target.filename,
        include_diff=False,
        include_exact_diff=False,
    )
    compared, total_accuracy, total_effective = report_function_accuracy(report)
    return {
        "target": target,
        "compared": compared,
        "accuracy": total_accuracy / compared if compared else 0.0,
        "effective_accuracy": total_effective / compared if compared else 0.0,
    }


def compare_vtables(repository: Path, target: str, class_filter: str | None) -> dict[str, Any]:
    from reccmp.compare.report import get_udiff_for_entity

    recmp_target = comparison_target(repository, target)
    warn_if_build_may_be_stale(repository, target, recmp_target)
    engine = Compare.from_target(recmp_target)
    name_filter = class_filter.casefold() if class_filter else None
    rows = []
    for item in engine.compare_vtables(include_diff=True):
        if name_filter is not None and name_filter not in (item.name or "").casefold():
            continue
        rows.append(
            {
                "name": item.name,
                "original": f"0x{item.orig_addr:08x}",
                "recompiled": (
                    f"0x{item.recomp_addr:08x}" if item.recomp_addr is not None else None
                ),
                "status": "exact" if item.accuracy == 1 else "mismatch",
                "accuracy": item.accuracy,
                "diff": get_udiff_for_entity(item),
            }
        )
    if not rows:
        qualifier = f" matching {class_filter!r}" if class_filter else ""
        raise RuntimeError(f"reccmp found zero vtables{qualifier}; refusing vacuous success")
    return {
        "ok": all(row["status"] == "exact" for row in rows),
        "count": len(rows),
        "exact_count": sum(row["status"] == "exact" for row in rows),
        "issue_count": sum(row["status"] != "exact" for row in rows),
        "filter": class_filter,
        "vtables": rows,
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
