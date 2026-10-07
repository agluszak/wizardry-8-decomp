"""Check ``src/surrender/sr.def`` against the reviewed retail export evidence."""

from __future__ import annotations

import csv
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from .binary.coff import export_directives, external_symbols
from .source_index import load_source_index

_DEF_PATH = Path("src/surrender/sr.def")
_EXPORTS_PATH = Path("evidence/snapshots/surrender-abi/exports.csv")
_PROGRAM = "--gog-base--sr--"
_MODULE = "sr.dll"
_IMAGE_BASE = 0x10000000

_DATA_KINDS = frozenset(
    {
        "vftable",
        "vbtable",
        "typeof",
        "string-literal",
        "local-static-guard",
        "rtti-type-descriptor",
        "rtti-base-class-descriptor",
        "rtti-base-class-array",
        "rtti-class-hierarchy-descriptor",
        "rtti-complete-object-locator",
    }
)
_ORDINAL = re.compile(r"^@(\d+)$")


class SurrenderExportsError(RuntimeError):
    """sr.def disagrees with the reviewed provider export evidence."""


@dataclass(frozen=True)
class DefEntry:
    name: str
    data: bool
    ordinal: int | None
    line: int


def _def_entries(path: Path) -> list[DefEntry]:
    entries: list[DefEntry] = []
    for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        text = line.split(";", 1)[0].strip()
        if not text or text.upper() in {"EXPORTS"} or text.upper().startswith("LIBRARY"):
            continue
        tokens = text.split()
        ordinal: int | None = None
        data = False
        for token in tokens[1:]:
            match = _ORDINAL.match(token)
            if match:
                ordinal = int(match.group(1))
            elif token.upper() == "DATA":
                data = True
        entries.append(DefEntry(name=tokens[0], data=data, ordinal=ordinal, line=number))
    return entries


def _evidence_exports(repository: Path) -> dict[str, dict[str, str]]:
    """Reviewed gog-base sr.dll exports keyed by decorated name."""

    rows: dict[str, dict[str, str]] = {}
    with (repository / _EXPORTS_PATH).open(encoding="utf-8", newline="") as handle:
        for row in csv.DictReader(handle):
            if row.get("module") != _MODULE or _PROGRAM not in str(row.get("program") or ""):
                continue
            name = str(row.get("decorated_name") or "")
            if name:
                rows[name] = row
    return rows


def _implicit_special_members(repository: Path, symbols: set[str]) -> set[str]:
    """Identify current compiler-owned special members, never original declarations.

    The Clang owner indexes authored declarations and deliberately omits implicit
    ones. Only reserved special-member symbols of an indexed, unchanged class
    without an authored declaration qualify. Ordinary/new APIs still fail.
    """
    from .paths import sha256_file

    candidates = {
        symbol: match[1]
        for symbol in symbols
        if (match := re.fullmatch(r"\?\?(?:0|1|4|_G|_E)([A-Za-z_][A-Za-z_0-9@]*?)@@.+", symbol))
    }
    if not candidates:
        return set()
    document = load_source_index(repository)
    authored = {row["semantic_id"] for row in document["declarations"]}
    classes = {
        row["qualified_name"]: row for row in document["classes"] if row["target"] == "SURRENDER"
    }
    result = set()
    for symbol in candidates.keys() - authored:
        owner = "::".join(reversed(candidates[symbol].split("@")))
        row = classes.get(owner)
        if row is None:
            continue
        source = row["source_file"]
        digest = document.get("source_digests", {}).get(source)
        if digest and sha256_file(repository / source) == digest:
            result.add(symbol)
    return result


def validate_surrender_provider_objects(repository: Path, objects: list[Path]) -> dict[str, Any]:
    """Require linkable retail bindings; retain implicit compiler export differences."""
    if not objects:
        raise SurrenderExportsError("no compiled SurRender provider objects")
    defined: set[str] = set()
    emitted: set[str] = set()
    for path in objects:
        defined.update(external_symbols(path)[0])
        emitted.update(export_directives(path.read_bytes()))
    required = {entry.name for entry in _def_entries(repository / _DEF_PATH)}
    evidence = _evidence_exports(repository)
    problems = [
        f"missing required export definition: {name}" for name in sorted(required - defined)
    ]
    excess = emitted - evidence.keys()
    implicit = _implicit_special_members(repository, excess) if excess else set()
    problems.extend(
        f"compiler export absent from retail: {name}" for name in sorted(excess - implicit)
    )
    if problems:
        raise SurrenderExportsError(
            "unresolved SurRender provider emissions:\n" + "\n".join(problems)
        )
    return {"ok": True, "compiler_exports_absent_from_retail": sorted(implicit)}


def _evidence_is_data(row: dict[str, str]) -> bool:
    kind = str(row.get("kind") or "")
    virtuality = str(row.get("virtuality") or "")
    return virtuality.endswith("-data") or kind in _DATA_KINDS or kind.startswith("rtti-")


def _established_names(repository: Path, evidence: dict[str, dict[str, str]]) -> dict[str, int]:
    """Retail exports bound by a SURRENDER source marker: name -> ordinal."""

    by_rva = {int(row["rva"], 16): row for row in evidence.values() if row.get("rva")}
    document = load_source_index(repository)
    established: dict[str, int] = {}
    for marker in document.get("markers") or []:
        if str(marker.get("target") or "").upper() != "SURRENDER":
            continue
        row = by_rva.get(int(marker["address"]) - _IMAGE_BASE)
        if row is not None:
            established[row["decorated_name"]] = int(row["ordinal"])
    return established


def validate_surrender_exports(repository: Path) -> dict[str, Any]:
    """Fail when sr.def loses or misstates an established provider export."""

    path = repository / _DEF_PATH
    entries = _def_entries(path)
    evidence = _evidence_exports(repository)
    established = _established_names(repository, evidence)

    lost: list[str] = []
    additions: list[str] = []
    data_kind: list[str] = []
    ordinals: list[str] = []
    duplicates: list[str] = []

    present = {entry.name for entry in entries}
    for name in sorted(established):
        if name not in present:
            lost.append(f"ordinal {established[name]}: {name}")

    seen: set[str] = set()
    for entry in entries:
        if entry.name in seen:
            duplicates.append(f"line {entry.line}: {entry.name}")
        seen.add(entry.name)
        row = evidence.get(entry.name)
        if row is None:
            additions.append(f"line {entry.line}: {entry.name}")
            continue
        if entry.data != _evidence_is_data(row):
            expected = "DATA" if _evidence_is_data(row) else "no DATA"
            data_kind.append(
                f"line {entry.line}: {entry.name} evidence kind "
                f"{row.get('kind')}/{row.get('virtuality')} requires {expected}"
            )
        if entry.ordinal is not None and entry.ordinal != int(row["ordinal"]):
            ordinals.append(
                f"line {entry.line}: {entry.name} declares @{entry.ordinal}, "
                f"evidence ordinal {row['ordinal']}"
            )

    if lost or additions or data_kind or ordinals or duplicates:
        sections = []
        if lost:
            sections.append("established exports removed from sr.def:\n  " + "\n  ".join(lost))
        if additions:
            sections.append(
                "sr.def names with no retail export evidence:\n  " + "\n  ".join(additions)
            )
        if data_kind:
            sections.append("sr.def DATA disagrees with evidence:\n  " + "\n  ".join(data_kind))
        if ordinals:
            sections.append("sr.def ordinal disagrees with evidence:\n  " + "\n  ".join(ordinals))
        if duplicates:
            sections.append("duplicate sr.def entries:\n  " + "\n  ".join(duplicates))
        raise SurrenderExportsError(
            "provider export table disagrees with reviewed retail evidence:\n" + "\n".join(sections)
        )
    return {
        "ok": True,
        "gate": "surrender-exports",
        "entries": len(entries),
        "established": len(established),
        "evidence": len(evidence),
    }


def built_export_disagreements(
    built: dict[str, int], evidence: dict[str, dict[str, str]]
) -> list[str]:
    """Named exports of a built sr.dll that retail does not have, lacks, or numbers differently.

    sr.def is only part of the provider surface: every member of a dllexport
    class is exported too, so an inline member added to an exported class
    widens the table without touching sr.def. The linker numbers exports by
    name, so one extra name early in the order renumbers everything after it.
    """
    problems = [
        f"not a retail export: @{built[name]} {name}"
        for name in sorted(built.keys() - evidence.keys())
    ]
    problems.extend(
        f"retail export not built: @{evidence[name]['ordinal']} {name}"
        for name in sorted(evidence.keys() - built.keys())
    )
    problems.extend(
        f"@{built[name]} {name}: retail ordinal {evidence[name]['ordinal']}"
        for name in sorted(built.keys() & evidence.keys())
        if built[name] != int(evidence[name]["ordinal"])
    )
    return problems


def validate_built_surrender_exports(repository: Path, dll: Path) -> dict[str, Any]:
    """Protect all retail names/ordinals and report extra implicit compiler emissions."""

    import pefile

    pe = pefile.PE(str(dll), fast_load=True)
    pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_EXPORT"]])
    directory = getattr(pe, "DIRECTORY_ENTRY_EXPORT", None)
    built = {
        symbol.name.decode("ascii"): symbol.ordinal
        for symbol in (directory.symbols if directory else [])
        if symbol.name
    }
    evidence = _evidence_exports(repository)
    implicit = _implicit_special_members(repository, built.keys() - evidence.keys())
    problems = built_export_disagreements(
        {name: ordinal for name, ordinal in built.items() if name not in implicit}, evidence
    )
    if problems:
        shown = problems[:40]
        more = len(problems) - len(shown)
        raise SurrenderExportsError(
            f"built {dll.name} export table disagrees with reviewed retail evidence:\n  "
            + "\n  ".join(shown)
            + (f"\n  ... and {more} more" if more else "")
        )
    return {
        "ok": True,
        "gate": "built-surrender-exports",
        "exports": len(built),
        "exact": not implicit,
        "compiler_exports_absent_from_retail": sorted(implicit),
    }
