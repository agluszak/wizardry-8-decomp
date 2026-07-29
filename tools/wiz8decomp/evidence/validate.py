from __future__ import annotations

import csv
import re
from collections.abc import Iterable, Mapping
from pathlib import Path

from ..provenance import ProvenanceError, validate_provenance
from .boundaries import load_boundary_rows
from .claims import validate_claim_rows
from .classes import load_reviewed_class_model
from .functions import load_function_identities
from .io import parse_hex, read_table
from .signatures import load_reviewed_signatures

_DIGEST = re.compile(r"[0-9a-f]{64}")


def validate_source_entries(categories: Mapping[str, Iterable[str]], root: Path) -> None:
    """Validate an explicit source inventory without deriving or ordering it."""
    seen: dict[str, str] = {}
    for category, entries in categories.items():
        for entry in entries:
            normalized = entry.replace("\\", "/")
            if normalized in seen:
                raise ValueError(
                    f"source {normalized} appears in both {seen[normalized]} and {category}"
                )
            if not (root / normalized).is_file():
                raise ValueError(f"{category} names missing source: {normalized}")
            seen[normalized] = category


def _validate_csv_shapes(repo_dir: Path) -> int:
    checked = 0
    for root_name in ("evidence", "config"):
        for path in sorted((repo_dir / root_name).rglob("*.csv")):
            with path.open(newline="", encoding="utf-8") as stream:
                reader = csv.DictReader(stream)
                header = reader.fieldnames
                if not header:
                    raise ValueError(f"{path}: missing CSV header")
                for line, row in enumerate(reader, start=2):
                    if None in row or len(row) != len(header):
                        raise ValueError(
                            f"{path}:{line}: row does not match its declared header; "
                            "write CSV through DictWriter"
                        )
            checked += 1
    if not checked:
        raise ValueError(f"{repo_dir}: no canonical evidence CSVs found")
    return checked


def _validate_functions(repo_dir: Path, program: str) -> set[int]:
    path = repo_dir / "evidence/reviewed" / program / "function-provenance.csv"
    table = read_table(path, program=program)
    addresses: set[int] = set()
    for line, row in enumerate(table.rows, start=2):
        address = parse_hex(row["address"], field="address", path=path) or 0
        if address <= 0:
            raise ValueError(f"{path}:{line}: address must be positive")
        try:
            validate_provenance(row["name_origin"], row["authority"])
        except ProvenanceError as error:
            raise ValueError(f"{path}:{line}: {error}") from error
        addresses.add(address)
    load_function_identities(path, program=program)
    return addresses


def _validate_boundaries(repo_dir: Path) -> set[int]:
    path = repo_dir / "config/reccmp/wiz8-gameplay-boundaries.csv"
    addresses: set[int] = set()
    for line, row in enumerate(load_boundary_rows(path), start=2):
        address = parse_hex(row["address"], field="address", path=path) or 0
        size = parse_hex(row["size"], field="size", path=path)
        if not size or size <= 0:
            raise ValueError(f"{path}:{line}: boundary size must be positive")
        digest = row["relocation_masked_sha256"].strip()
        if row["confidence"].strip() == "exact" and not _DIGEST.fullmatch(digest):
            raise ValueError(f"{path}:{line}: exact boundary requires a SHA-256 digest")
        addresses.add(address)
    return addresses


def _validate_class_references(repo_dir: Path, program: str, functions: set[int]) -> None:
    model = load_reviewed_class_model(repo_dir, program)
    classes = {item.name for item in model.classes}
    for reviewed_class in model.classes:
        for address in (
            reviewed_class.constructor,
            reviewed_class.destructor,
            reviewed_class.scalar_deleting_destructor,
        ):
            if address is not None and address not in functions:
                raise ValueError(
                    f"class {reviewed_class.name} lifecycle address {address:08x} "
                    "does not resolve to canonical function evidence"
                )
    for slot in model.slots:
        if slot.target not in functions:
            raise ValueError(
                f"{slot.vtable_id} slot {slot.index} target {slot.target:08x} "
                "does not resolve to canonical function evidence"
            )
        prefix = f"classes:{program}:"
        if slot.evidence_id.startswith(prefix) and slot.evidence_id[len(prefix) :] not in classes:
            raise ValueError(f"{slot.vtable_id} slot {slot.index} has dangling {slot.evidence_id}")


def _validate_signatures(repo_dir: Path, program: str, functions: set[int]) -> None:
    for signature in load_reviewed_signatures(repo_dir, program):
        if signature.address not in functions:
            raise ValueError(
                f"signature {signature.evidence_id} address {signature.address:08x} "
                "does not resolve to functions.csv"
            )


def _observed_function_addresses(repo_dir: Path) -> set[int]:
    path = repo_dir / "evidence/snapshots/functions/candidates.csv"
    with path.open(newline="", encoding="utf-8") as stream:
        return {
            int(row["address"], 16)
            for row in csv.DictReader(stream)
            if "--gog-base--" in row["program"]
        }


def _validate_polymorphism_observation(repo_dir: Path, program: str) -> int:
    model = load_reviewed_class_model(repo_dir, program)
    census_path = repo_dir / "evidence/snapshots/polymorphism/slots.csv"
    with census_path.open(newline="", encoding="utf-8") as stream:
        census = {
            (row["vtable"], int(row["slot_index"])): row["target"]
            for row in csv.DictReader(stream)
            if "--gog-base--" in row["program"]
        }
    addresses = {item.vtable_id: f"{item.address:08x}" for item in model.vtables}
    checked = 0
    for slot in model.slots:
        expected = census.get((addresses[slot.vtable_id], slot.index))
        if expected is None:
            continue
        checked += 1
        actual = f"{slot.target:08x}"
        if actual != expected:
            raise ValueError(
                f"{slot.vtable_id} slot {slot.index} records {actual}; image holds {expected}"
            )
    if not checked:
        raise ValueError("reviewed vtable slots have no overlap with the polymorphism census")
    return checked


def validate_repository(repo_dir: Path, program: str = "wiz8") -> dict[str, object]:
    """Validate canonical evidence relationships without pinning corpus progress totals."""
    checks: list[dict[str, object]] = []

    def run(name: str, operation) -> object:
        try:
            detail = operation()
        except (OSError, ValueError, RuntimeError) as error:
            checks.append({"name": name, "ok": False, "error": str(error)})
            return None
        display = {"count": len(detail)} if isinstance(detail, set) else detail
        checks.append({"name": name, "ok": True, "detail": display})
        return detail

    run("csv-shapes", lambda: {"files": _validate_csv_shapes(repo_dir)})
    functions = run("reviewed-functions", lambda: _validate_functions(repo_dir, program))
    boundaries = run("reviewed-boundaries", lambda: _validate_boundaries(repo_dir))
    if isinstance(functions, set):
        run(
            "reviewed-claims", lambda: {"claims": validate_claim_rows(repo_dir, functions, program)}
        )
        canonical_addresses = (
            functions
            | (boundaries if isinstance(boundaries, set) else set())
            | _observed_function_addresses(repo_dir)
        )
        run(
            "reviewed-classes",
            lambda: _validate_class_references(repo_dir, program, canonical_addresses),
        )
        run(
            "reviewed-signatures",
            lambda: _validate_signatures(repo_dir, program, functions),
        )
    run(
        "reviewed-polymorphism-observation",
        lambda: {"slots": _validate_polymorphism_observation(repo_dir, program)},
    )
    failures = [check for check in checks if not check["ok"]]
    return {"ok": not failures, "checks": checks, "failure_count": len(failures)}


def require_valid_repository(repo_dir: Path, program: str = "wiz8") -> dict[str, object]:
    report = validate_repository(repo_dir, program)
    if not report["ok"]:
        checks = report["checks"]
        assert isinstance(checks, list)
        messages = [str(check["error"]) for check in checks if not check["ok"]]
        raise ValueError("repository validation failed: " + "; ".join(messages))
    return report
