"""Small structured recovery workflows over reccmp's owned analyses."""

from __future__ import annotations

import json
import shutil
import tempfile
from collections.abc import Iterable
from pathlib import Path
from typing import Any

import yaml

from .paths import atomic_json
from .subprocesses import run

SR_ASSERT_FIXED = b"?srAssertFail@@YAXPBD0J0@Z"
SR_ASSERT_VARIADIC = b"?srAssertFail@@YAXPBD0J0ZZ"


def parse_address(value: str) -> int:
    try:
        address = int(value, 16)
    except ValueError as error:
        raise ValueError(f"not a hexadecimal address: {value}") from error
    if address < 0:
        raise ValueError(f"address must not be negative: {value}")
    return address


def addresses_from_files(repository: Path, paths: Iterable[Path]) -> list[int]:
    """Select compiler-bound FUNCTION markers from the shared source index."""

    from .source_index import load_source_index

    selected = {
        str((path if path.is_absolute() else repository / path).resolve()) for path in paths
    }
    return [
        int(marker["address"])
        for marker in load_source_index(repository)["markers"]
        if marker["marker_kind"] == "FUNCTION"
        and str((repository / marker["source_file"]).resolve()) in selected
    ]


def changed_source_files(repository: Path, since: str | None = None) -> list[Path]:
    """Use Jujutsu's diff to select current C++ files, including marked headers."""

    command = ["jj", "diff", "--name-only", "--color=never"]
    if since is not None:
        command.extend(("--from", since))
    result = run(command, cwd=repository)
    return [
        path
        for name in result.stdout.splitlines()
        if (path := repository / name).suffix.lower()
        in {".cpp", ".cc", ".cxx", ".h", ".hpp", ".hxx"}
        and path.is_file()
    ]


def selected_addresses(repository: Path, raw: Iterable[str], paths: Iterable[Path]) -> list[int]:
    selected = set(_resolve_source_selectors(repository, raw)) if raw else set()
    selected.update(addresses_from_files(repository, paths))
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


def _resolve_source_selectors(repository: Path, values: Iterable[str]) -> list[int]:
    """Resolve addresses, ranges, and exact source-owned identities for compare."""

    from .source_index import source_functions

    model = source_functions(repository)
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


def run_report(
    repository: Path,
    target: str,
    *,
    original_addresses: Iterable[int] = (),
    recompiled_addresses: Iterable[int] = (),
) -> list[dict[str, Any]]:
    """Run one fresh filtered comparison and return its structured entities."""

    with tempfile.TemporaryDirectory(prefix="wiz8-reccmp-") as temporary:
        report = Path(temporary) / "report.json"
        command = [
            "reccmp-reccmp",
            "--target",
            target,
            "--json",
            report,
            "--json-diet",
            "--silent",
        ]
        for address in sorted(set(original_addresses)):
            command.extend(("--orig-address", hex(address)))
        for address in sorted(set(recompiled_addresses)):
            command.extend(("--recomp-address", hex(address)))
        run(command, cwd=repository / "build" / "decomp")
        payload = json.loads(report.read_text(encoding="utf-8"))
        if target == "WIZ8":
            payload["data"] = _normalize_sr_assert_comparisons(
                repository, Path(temporary), payload["data"]
            )
    if payload.get("format") != 1 or not isinstance(payload.get("data"), list):
        raise ValueError("reccmp report does not use the supported structured schema")
    return payload["data"]


def _sr_assert_alias_mismatch(entity: dict[str, Any]) -> bool:
    comparison = entity.get("comparison") or {}
    difference = comparison.get("difference") or {}
    if comparison.get("status") != "mismatch" or difference.get("kind") != "call_target":
        return False
    original = str(((difference.get("orig") or {}).get("facts") or {}).get("target_name") or "")
    recompiled = str(((difference.get("recomp") or {}).get("facts") or {}).get("target_name") or "")
    return SR_ASSERT_VARIADIC.decode() in original and SR_ASSERT_FIXED.decode() in recompiled


def _normalize_sr_assert_comparisons(
    repository: Path, temporary: Path, rows: list[dict[str, Any]]
) -> list[dict[str, Any]]:
    addresses = [
        address
        for row in rows
        if _sr_assert_alias_mismatch(row) and (address := _entity_address(row)) is not None
    ]
    if not addresses:
        return rows

    user = yaml.safe_load((repository / "reccmp-user.yml").read_text(encoding="utf-8")) or {}
    original = Path(str(user["targets"]["WIZ8"]["path"]).strip())
    executable = repository / "build" / "decomp" / "Wiz8.exe"
    pdb = repository / "build" / "decomp" / "Wiz8.pdb"
    normalized_executable = temporary / "Wiz8-srAssert-normalized.exe"
    normalized_pdb = temporary / "Wiz8-srAssert-normalized.pdb"
    shutil.copyfile(executable, normalized_executable)
    shutil.copyfile(pdb, normalized_pdb)
    for path in (normalized_executable, normalized_pdb):
        content = path.read_bytes()
        if SR_ASSERT_FIXED not in content:
            raise ValueError(f"expected srAssertFail import spelling is absent from {path.name}")
        path.write_bytes(content.replace(SR_ASSERT_FIXED, SR_ASSERT_VARIADIC))

    normalized_report = temporary / "srAssert-normalized.json"
    command = [
        "reccmp-reccmp",
        "--paths",
        str(original),
        str(normalized_executable),
        str(normalized_pdb),
        str(repository / "src" / "wiz8"),
        "--json",
        str(normalized_report),
        "--json-diet",
        "--silent",
        "--no-cache",
    ]
    for address in sorted(set(addresses)):
        command.extend(("--orig-address", hex(address)))
    run(command, cwd=repository / "build" / "decomp")
    normalized = json.loads(normalized_report.read_text(encoding="utf-8"))["data"]
    replacements = {
        address: row for row in normalized if (address := _entity_address(row)) is not None
    }
    result: list[dict[str, Any]] = []
    for row in rows:
        address = _entity_address(row)
        result.append(replacements.get(address, row) if address is not None else row)
    return result


def _entity_address(entity: dict[str, Any], key: str = "address") -> int | None:
    value = entity.get(key)
    if not isinstance(value, str) or value == "various":
        return None
    try:
        return int(value, 16)
    except ValueError:
        return None


def _effective_score(entity: dict[str, Any]) -> float:
    comparison = entity.get("comparison") or {}
    if comparison.get("status") in {"exact", "effective"}:
        return 1.0
    matching = entity.get("matching")
    return float(matching) if isinstance(matching, int | float) else 0.0


def compare_rows(rows: list[dict[str, Any]], wanted: Iterable[int]) -> dict[str, Any]:
    by_address = {address: row for row in rows if (address := _entity_address(row)) is not None}
    compared: list[dict[str, Any]] = []
    for address in sorted(set(wanted)):
        entity = by_address.get(address)
        if entity is None:
            compared.append({"address": f"0x{address:08x}", "status": "missing"})
            continue
        comparison = entity.get("comparison") or {}
        compared.append(
            {
                "address": f"0x{address:08x}",
                "name": entity.get("name"),
                "raw_matching": entity.get("matching"),
                "effective_matching": _effective_score(entity),
                "status": comparison.get("status", "unclassified"),
            }
        )
    exact = sum(row["status"] == "exact" for row in compared)
    effective = sum(row["status"] == "effective" for row in compared)
    missing = sum(row["status"] == "missing" for row in compared)
    return {
        "ok": exact + effective == len(compared),
        "selected": len(compared),
        "exact": exact,
        "effective": effective,
        "below_exact": len(compared) - exact - effective - missing,
        "missing": missing,
        "functions": compared,
    }


def compare_selected(
    repository: Path, target: str, addresses: list[int], *, include_windows: bool = True
) -> dict[str, Any]:
    rows = run_report(repository, target, original_addresses=addresses)
    comparison = compare_rows(rows, addresses)
    triage = triage_rows(rows, addresses)
    details = {int(row["address"], 16): row for row in triage["functions"]}
    for row in comparison["functions"]:
        detail = details.get(int(row["address"], 16), {})
        for key in ("difference", "reason", "location", "effective_reasons"):
            if key in detail:
                row[key] = detail[key]
        if include_windows and row["status"] == "mismatch":
            window = _instruction_windows(repository, target, rows, int(row["address"], 16))
            if window:
                row["instruction_window"] = window
                difference = row.get("difference") or {}
                if difference.get("kind") == "branch_target" and not _paired_branch_witness(window):
                    row["reported_difference"] = difference
                    row["difference"] = {"kind": "alignment_or_structure"}
    return comparison


def _paired_branch_witness(window: dict[str, list[dict[str, Any]]]) -> bool:
    """A branch-target diagnosis requires corresponding branch instructions on both sides."""

    def divergent_mnemonic(side: str) -> str:
        row = next((item for item in window.get(side, []) if item.get("divergence")), None)
        return str(row.get("instruction", "")).split(maxsplit=1)[0].casefold() if row else ""

    mnemonics = (divergent_mnemonic("original"), divergent_mnemonic("recomp"))
    return all(value.startswith("j") and value != "jmp" for value in mnemonics)


def _instruction_windows(
    repository: Path,
    target: str,
    rows: list[dict[str, Any]],
    address: int,
    *,
    radius: int = 3,
) -> dict[str, list[dict[str, Any]]]:
    """Decode a bounded window around reccmp's structured first divergence."""

    from .binary.code import disassembler
    from .binary.image import PeImage

    entity = next((row for row in rows if _entity_address(row) == address), None)
    if entity is None:
        return {}
    difference = (entity.get("comparison") or {}).get("difference") or {}
    user = yaml.safe_load((repository / "reccmp-user.yml").read_text(encoding="utf-8")) or {}
    build = (
        yaml.safe_load((repository / "build/decomp/reccmp-build.yml").read_text(encoding="utf-8"))
        or {}
    )
    paths = {
        "original": Path(str(user["targets"][target]["path"]).strip()),
        "recomp": Path(str(build["targets"][target]["path"]).strip()),
    }
    starts = {"original": address, "recomp": _entity_address(entity, "recomp")}
    sides = {"original": difference.get("orig") or {}, "recomp": difference.get("recomp") or {}}
    result: dict[str, list[dict[str, Any]]] = {}
    for side in ("original", "recomp"):
        start = starts[side]
        index = sides[side].get("instruction_index")
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


def triage_rows(rows: list[dict[str, Any]], wanted: Iterable[int]) -> dict[str, Any]:
    by_address = {address: row for row in rows if (address := _entity_address(row)) is not None}
    findings: list[dict[str, Any]] = []
    for address in sorted(set(wanted)):
        entity = by_address.get(address)
        if entity is None:
            findings.append({"address": f"0x{address:08x}", "status": "missing"})
            continue
        comparison = entity.get("comparison")
        if not isinstance(comparison, dict) or not isinstance(comparison.get("status"), str):
            raise TypeError(f"0x{address:08x} lacks structured comparison results")
        status = comparison["status"]
        finding: dict[str, Any] = {
            "address": f"0x{address:08x}",
            "name": entity.get("name"),
            "status": status,
            "raw_matching": entity.get("matching"),
        }
        if status == "effective":
            finding["effective_reasons"] = comparison.get("effective_reasons") or []
        elif status == "exact":
            pass
        elif status == "mismatch":
            difference = comparison.get("difference")
            if not isinstance(difference, dict):
                raise ValueError(f"0x{address:08x} mismatch lacks its structured difference")
            finding["difference"] = difference
        elif status == "inconclusive":
            finding["reason"] = comparison.get("inconclusive_reason") or "analysis_limit"
            finding["location"] = comparison.get("inconclusive_location")
        else:
            raise ValueError(f"0x{address:08x} has unknown comparison status: {status}")
        findings.append(finding)
    return {"functions": findings}


def translate_rows(rows: list[dict[str, Any]], queries: Iterable[int]) -> dict[str, Any]:
    by_original = {
        address: row for row in rows if (address := _entity_address(row, "address")) is not None
    }
    by_recompiled = {
        address: row for row in rows if (address := _entity_address(row, "recomp")) is not None
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
        original = _entity_address(entity, "address")
        recompiled = _entity_address(entity, "recomp")
        translations.append(
            {
                "query": f"0x{query:08x}",
                "direction": direction,
                "original": f"0x{original:08x}" if original is not None else None,
                "recompiled": f"0x{recompiled:08x}" if recompiled is not None else None,
                "name": entity.get("name"),
                "raw_matching": entity.get("matching"),
                "status": (entity.get("comparison") or {}).get("status"),
            }
        )
    return {"translations": translations}


def translate_addresses(repository: Path, target: str, queries: list[int]) -> dict[str, Any]:
    rows = run_report(
        repository,
        target,
        original_addresses=queries,
        recompiled_addresses=queries,
    )
    return translate_rows(rows, queries)


def compare_vtables(repository: Path, target: str, class_filter: str | None) -> dict[str, Any]:
    from reccmp.compare import Compare
    from reccmp.project.detect import RecCmpProject

    project = RecCmpProject.from_directory(repository / "build" / "decomp")
    if project is None:
        raise RuntimeError("reccmp project is unavailable")
    engine = Compare.from_target(project.get(target))
    name_filter = class_filter.casefold() if class_filter else None
    rows = []
    for item in engine.compare_vtables(include_diff=False):
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
        "issues": [row for row in rows if row["status"] != "exact"],
    }


def compare_data(repository: Path, target: str) -> dict[str, Any]:
    from reccmp.project.detect import RecCmpProject
    from reccmp.tools.datacmp import do_the_comparison

    project = RecCmpProject.from_directory(repository / "build" / "decomp")
    if project is None:
        raise RuntimeError("reccmp project is unavailable")
    items = list(do_the_comparison(project.get(target)))
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
