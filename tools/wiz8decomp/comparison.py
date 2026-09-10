"""Focused comparison and address-translation workflows over reccmp's API."""

from __future__ import annotations

from collections.abc import Iterable
from dataclasses import asdict
from pathlib import Path
from typing import Any

from reccmp.compare import Compare
from reccmp.compare.report import ReccmpComparedEntity
from reccmp.project.detect import RecCmpProject, RecCmpTarget

from .paths import atomic_json
from .subprocesses import run


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


def changed_source_files(repository: Path, since: str | None = None) -> list[Path]:
    """Use Jujutsu's diff to select current C/C++ files, including marked headers."""

    command = ["jj", "diff", "--name-only", "--color=never"]
    if since is not None:
        command.extend(("--from", since))
    result = run(command, cwd=repository)
    return [
        path
        for name in result.stdout.splitlines()
        if (path := repository / name).suffix.lower()
        in {".c", ".cpp", ".cc", ".cxx", ".h", ".hpp", ".hxx"}
        and path.is_file()
    ]


def selected_addresses(
    repository: Path, target: str, raw: Iterable[str], paths: Iterable[Path]
) -> list[int]:
    selected = set(_resolve_source_selectors(repository, target, raw)) if raw else set()
    selected.update(addresses_from_files(repository, target, paths))
    if not selected:
        raise ValueError("pass one or more addresses and/or --file source paths")
    return sorted(selected)


def _numeric_range(value: str) -> tuple[int, int] | None:
    start_text, separator, end_text = value.strip().partition(":")
    try:
        start = int(start_text, 0)
        end = int(end_text, 0) if separator else start
    except ValueError:
        return None
    if start < 0 or end < start:
        raise ValueError(f"invalid function selector range: {value}")
    return start, end


def _resolve_source_selectors(repository: Path, target: str, values: Iterable[str]) -> list[int]:
    """Resolve addresses, ranges, and exact source-owned identities for compare."""

    from .source_index import source_functions

    model = source_functions(repository, target)
    selected: set[int] = set()
    by_name: dict[str, list[int]] = {}
    for address, function in model.items():
        by_name.setdefault(function.name, []).append(address)

    for value in values:
        numeric = _numeric_range(value)
        if numeric is not None:
            start, end = numeric
            if start == end:
                selected.add(start)
            else:
                matches = [address for address in model if start <= address <= end]
                if not matches:
                    raise ValueError(f"no source-owned functions in selector range {value}")
                selected.update(matches)
            continue
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
    repository: Path, target: str, addresses: list[int], *, include_windows: bool = True
) -> dict[str, Any]:
    recmp_target = _project(repository).get(target)
    engine = Compare.from_target(recmp_target, orig_addrs=addresses)
    matches = {
        entity.orig_addr: entity
        for entity in engine.compare_addresses(
            orig_addrs=addresses,
            include_diff=False,
            include_exact_diff=False,
        )
    }
    functions: list[dict[str, Any]] = []
    for address in sorted(set(addresses)):
        entity = matches.get(address)
        if entity is None:
            functions.append({"address": f"0x{address:08x}", "status": "missing"})
            continue
        row = _function_result(entity)
        if include_windows and row["status"] == "mismatch":
            window = _instruction_windows(recmp_target, entity)
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
    return {
        "ok": exact + effective == len(functions),
        "selected": len(functions),
        "exact": exact,
        "effective": effective,
        "below_exact": len(functions) - exact - effective - missing,
        "missing": missing,
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
            image = PeImage(paths[side])
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
    recmp_target = _project(repository).get(target)
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

    engine = Compare.from_target(_project(repository).get(target))
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

    items = list(do_the_comparison(_project(repository).get(target)))
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
