from __future__ import annotations

import csv
from collections.abc import Iterable, Mapping
from pathlib import Path

from ..provenance import ProvenanceError, validate_provenance
from ..source_model import build_source_model, validate_source_index
from .claims import load_claims, validate_claim_rows
from .io import parse_hex


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
    source_addresses = set(build_source_model(repo_dir, program.upper()).functions)
    claims_path = repo_dir / "evidence" / "reviewed" / program / "claims.csv"
    reviewed_addresses = {
        parse_hex(claim["entity_key"], field="entity_key", path=claims_path) or 0
        for claim in load_claims(repo_dir, program)
        if claim["entity_kind"].strip() == "function"
    }
    addresses = source_addresses | reviewed_addresses
    if not addresses or 0 in addresses:
        raise ValueError("function source/claim model contains an invalid address")
    return addresses


def _validate_function_catalogs(repo_dir: Path) -> int:
    """Validate exceptional external function catalogs and their provenance."""

    checked = 0
    for path in sorted((repo_dir / "evidence/reviewed").glob("*/functions.csv")):
        seen: set[tuple[str, str]] = set()
        path_count = 0
        with path.open(newline="", encoding="utf-8") as stream:
            for line, row in enumerate(csv.DictReader(stream), start=2):
                identity = (row["program"], row["address"])
                if identity in seen:
                    raise ValueError(f"{path}:{line}: duplicate function identity {identity}")
                if row["program"] != path.parent.name:
                    raise ValueError(
                        f"{path}:{line}: program {row['program']!r} "
                        f"does not match {path.parent.name!r}"
                    )
                try:
                    validate_provenance(row["name_origin"], row["authority"])
                except (KeyError, ProvenanceError) as error:
                    raise ValueError(f"{path}:{line}: {error}") from error
                seen.add(identity)
                path_count += 1
                checked += 1
        if not path_count:
            raise ValueError(f"{path}: function catalog is empty")
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
    run("function-provenance", lambda: {"functions": _validate_function_catalogs(repo_dir)})
    run("source-index", lambda: validate_source_index(repo_dir))
    functions = run("source-functions", lambda: _validate_functions(repo_dir, program))
    if isinstance(functions, set):
        run(
            "reviewed-claims", lambda: {"claims": validate_claim_rows(repo_dir, functions, program)}
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
