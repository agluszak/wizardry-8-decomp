"""Small structured recovery workflows over reccmp's owned analyses."""

from __future__ import annotations

import json
import re
import tempfile
from collections.abc import Iterable
from pathlib import Path
from typing import Any

from .subprocesses import run

VTABLE_COUNT = re.compile(r"Vtables found:\s*(?P<count>\d+)\.")


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

    from .source_model import load_source_index

    selected = {
        str((path if path.is_absolute() else repository / path).resolve()) for path in paths
    }
    return [
        int(marker["address"])
        for marker in load_source_index(repository)["markers"]
        if marker["marker_kind"] == "FUNCTION"
        and str((repository / marker["source_file"]).resolve()) in selected
    ]


def selected_addresses(repository: Path, raw: Iterable[str], paths: Iterable[Path]) -> list[int]:
    selected = {parse_address(value) for value in raw}
    selected.update(addresses_from_files(repository, paths))
    if not selected:
        raise ValueError("pass one or more addresses and/or --file source paths")
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
    if payload.get("format") != 1 or not isinstance(payload.get("data"), list):
        raise ValueError("reccmp report does not use the supported structured schema")
    return payload["data"]


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


def compare_selected(repository: Path, target: str, addresses: list[int]) -> dict[str, Any]:
    return compare_rows(
        run_report(repository, target, original_addresses=addresses),
        addresses,
    )


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


def triage_selected(repository: Path, target: str, addresses: list[int]) -> dict[str, Any]:
    return triage_rows(
        run_report(repository, target, original_addresses=addresses),
        addresses,
    )


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
