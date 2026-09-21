"""Check ``src/surrender/sr.def`` against the reviewed retail export evidence.

The provider DEF is deliberately a subset of the original 2059 exports -
member ``dllexport`` declarations contribute the rest - but every entry it
carries is an established provider fact. The reviewed export table
(``evidence/snapshots/surrender-abi/exports.csv``, ``sr.dll`` rows of the
gog-base program) therefore bounds what sr.def may say:

- an entry whose decorated name is not a retail export is an addition, not an
  established export;
- a ``DATA`` keyword is required exactly where the evidence row describes data
  (``*-data`` members, vftables, vbtables, RTTI records, literals);
- a pinned ``@ordinal`` must agree with the retail ordinal.

Loss runs the other way: a retail export whose address is bound by a
SURRENDER source marker is part of the recovered provider surface, so its
name must stay in sr.def. Marker kinds all count - an exported vtable or a
compiler emission is still an established export.
"""

from __future__ import annotations

import csv
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Any

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
