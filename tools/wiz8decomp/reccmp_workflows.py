"""Small structured recovery workflows over reccmp's owned analyses."""

from __future__ import annotations

import json
import re
import shutil
import tempfile
from collections.abc import Iterable
from pathlib import Path
from typing import Any

import yaml

from .subprocesses import run

VTABLE_COUNT = re.compile(r"Vtables found:\s*(?P<count>\d+)\.")
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
        for key in ("difference", "guidance", "reason", "location", "conclusion"):
            if key in detail:
                row[key] = detail[key]
        if include_windows and row["status"] == "mismatch":
            window = _instruction_windows(repository, target, rows, int(row["address"], 16))
            if window:
                row["instruction_window"] = window
    return comparison


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


def comparison_human(result: dict[str, Any]) -> str:
    lines: list[str] = []
    for row in result["functions"]:
        address = row["address"].removeprefix("0x").upper()
        status = row["status"]
        name = row.get("name") or ""
        if status in {"exact", "effective"}:
            lines.append(f"{address} {name} {status}".rstrip())
            continue
        score = row.get("raw_matching")
        score_text = f" {float(score) * 100:.1f}%" if isinstance(score, int | float) else ""
        lines.append(f"{address} {name} {status}{score_text}".rstrip())
        difference = row.get("difference") or {}
        if difference:
            lines.append(f"  first divergence: {difference.get('kind', 'unknown')}")
        if row.get("guidance"):
            lines.append(f"  {row['guidance']}")
        for side in ("original", "recomp"):
            window = (row.get("instruction_window") or {}).get(side, [])
            if not window:
                continue
            lines.append(f"  {side}:")
            for instruction in window:
                marker = ">" if instruction["divergence"] else " "
                lines.append(
                    f"  {marker} {instruction['address'].removeprefix('0x').upper()}  "
                    f"{instruction['instruction']}"
                )
    return "\n".join(lines)


def _guidance(kind: str) -> str:
    return {
        "memory_address": "inspect the typed field/global selection and object layout",
        "memory_value": "trace the stored value's typed source expression",
        "call_target": "inspect callee identity, ownership, and dispatch",
        "call_argument": "inspect receiver and argument provenance at the callsite",
        "branch_condition": "inspect the comparison operands and signedness",
        "branch_target": "inspect early-return and source block structure",
        "return_value": "inspect the declared return type and returned expression",
        "preserved_state": "inspect local lifetimes and prologue/epilogue shape",
    }.get(kind, "inspect the first structured machine-state divergence")


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
            finding["conclusion"] = "proved semantically harmless"
        elif status == "exact":
            finding["conclusion"] = "no divergence"
        elif status == "mismatch":
            difference = comparison.get("difference")
            if not isinstance(difference, dict):
                raise ValueError(f"0x{address:08x} mismatch lacks its structured difference")
            kind = str(difference.get("kind") or "unknown")
            finding["difference"] = difference
            finding["guidance"] = _guidance(kind)
        elif status == "inconclusive":
            finding["reason"] = comparison.get("inconclusive_reason") or "analysis_limit"
            finding["location"] = comparison.get("inconclusive_location")
            finding["conclusion"] = "not evidence of a source defect"
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


def vtable_count(output: str) -> int:
    match = VTABLE_COUNT.search(output)
    if match is None:
        raise ValueError("reccmp-vtable did not report an entity count")
    return int(match.group("count"))


def compare_vtables(
    repository: Path, target: str, class_filter: str | None, *, verbose: bool = False
) -> dict[str, Any]:
    command = ["reccmp-vtable", "--target", target, "--no-color"]
    if class_filter:
        command.extend(("--filter", class_filter))
    if verbose:
        command.append("--verbose")
    result = run(command, cwd=repository / "build" / "decomp", check=False)
    count = vtable_count(result.stdout)
    if count == 0:
        qualifier = f" matching {class_filter!r}" if class_filter else ""
        raise RuntimeError(f"reccmp found zero vtables{qualifier}; refusing vacuous success")
    return {
        "ok": result.exit_status == 0,
        "vtables": count,
        "filter": class_filter,
        "output": result.stdout,
        "diagnostics": result.stderr,
    }


def compare_data(
    repository: Path, target: str, *, show_all: bool = False, verbose: bool = False
) -> dict[str, Any]:
    command = ["reccmp-datacmp", "--target", target, "--no-color"]
    if show_all:
        command.append("--all")
    if verbose:
        command.append("--verbose")
    result = run(command, cwd=repository / "build" / "decomp", check=False)
    return {
        "ok": result.exit_status == 0,
        "output": result.stdout,
        "diagnostics": result.stderr,
    }
