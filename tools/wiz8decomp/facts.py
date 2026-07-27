"""The disposable whole-program fact store.

Every evidence channel in this repository currently terminates in a file a
human reads. This store is where they terminate in something an analysis can
consume: one SQLite database under `build/`, holding entities, relations,
tiers and provenance, rebuilt from the canonical inputs on demand and never
tracked. The evidence files stay canonical; this is an index, and deleting it
loses nothing.

Two properties matter more than the schema:

* Every fact names its origin - the CSV row or address it came from - and a
  derived fact carries edges to every parent it was derived from, so `why`
  answers with a chain instead of a shrug, and a candidate's consequences stay
  dependent on the candidate hypothesis instead of laundering into evidence.
* Rebuilds are deterministic: the same inputs produce byte-identical content,
  which is what makes the store safe to throw away.

Tiers are `observation` (machine census), `candidate` (hypothesis), `reviewed`
(human-accepted), `exact` (byte-proven). Nothing in this module promotes.
"""

from __future__ import annotations

import csv
import json
import sqlite3
from collections.abc import Iterable
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from .config import Settings

_SCHEMA = """
CREATE TABLE fact (
    id INTEGER PRIMARY KEY,
    kind TEXT NOT NULL,
    subject TEXT NOT NULL,
    object TEXT,
    tier TEXT NOT NULL CHECK (tier IN ('observation', 'candidate', 'reviewed', 'exact')),
    program TEXT,
    source TEXT NOT NULL,
    attributes TEXT NOT NULL DEFAULT '{}'
);
CREATE TABLE derivation (
    fact INTEGER NOT NULL REFERENCES fact (id),
    parent INTEGER NOT NULL REFERENCES fact (id),
    PRIMARY KEY (fact, parent)
);
CREATE INDEX fact_subject ON fact (subject);
CREATE INDEX fact_kind ON fact (kind, subject);
CREATE INDEX fact_object ON fact (object) WHERE object IS NOT NULL;
"""

_CANONICAL = "wiz8--gog-base--wiz8--18a74ff61c65"


@dataclass(frozen=True)
class Fact:
    kind: str
    subject: str
    object: str | None
    tier: str
    program: str | None
    source: str
    attributes: dict[str, Any]


def store_path(settings: Settings) -> Path:
    return settings.build_dir / "facts" / "facts.sqlite"


def _confidence_tier(confidence: str) -> str:
    return "exact" if confidence.strip() == "exact" else "reviewed"


def _rows(path: Path) -> Iterable[tuple[int, dict[str, str]]]:
    with path.open(newline="", encoding="utf-8") as stream:
        yield from enumerate(csv.DictReader(stream), start=2)


def _reviewed_facts(repo: Path) -> Iterable[Fact]:
    base = repo / "evidence" / "reviewed" / "wiz8"

    for line, row in _rows(base / "functions.csv"):
        if not row["address"]:
            continue
        source = f"evidence/reviewed/wiz8/functions.csv:{line}"
        subject = f"function:{row['program']}:{row['address']}"
        yield Fact(
            "function-identity",
            subject,
            None,
            _confidence_tier(row["confidence"]),
            row["program"],
            source,
            {
                "name": row["current_name"] or row["provisional_name"],
                "owner": row["owner"],
                "name_origin": row["name_origin"],
                "size": row["size"] or None,
            },
        )
        if row["source_path"]:
            yield Fact(
                "belongs-to-translation-unit",
                subject,
                f"unit:{row['source_path']}",
                _confidence_tier(row["confidence"]),
                row["program"],
                source,
                {},
            )

    for line, row in _rows(base / "classes.csv"):
        source = f"evidence/reviewed/wiz8/classes.csv:{line}"
        subject = f"type:{row['class_name']}"
        yield Fact(
            "type-identity",
            subject,
            None,
            _confidence_tier(row["confidence"]),
            row["program"],
            source,
            {
                "size": row["minimum_size"] or None,
                "primary_vtable": row["primary_vtable_id"] or None,
                "bases": row["base_classes"] or None,
            },
        )
        if row["source_path"]:
            yield Fact(
                "belongs-to-translation-unit",
                subject,
                f"unit:{row['source_path']}",
                _confidence_tier(row["confidence"]),
                row["program"],
                source,
                {},
            )

    for line, row in _rows(base / "fields.csv"):
        yield Fact(
            "has-field-at-offset",
            f"type:{row['class_name']}",
            f"field:{row['class_name']}+{row['offset']}",
            _confidence_tier(row["confidence"]),
            row["program"],
            f"evidence/reviewed/wiz8/fields.csv:{line}",
            {
                "offset": row["offset"],
                "size": row["size"],
                "name": row["field_name"],
                "data_type": row["data_type"],
                "pointee": row["pointee"] or None,
            },
        )

    for line, row in _rows(base / "vtables.csv"):
        yield Fact(
            "vtable-instance",
            f"vtable:{row['program']}:{row['address']}",
            f"type:{row['class_name']}",
            _confidence_tier(row["confidence"]),
            row["program"],
            f"evidence/reviewed/wiz8/vtables.csv:{line}",
            {
                "vtable_id": row["vtable_id"],
                "subobject_offset": row["subobject_offset"],
                "kind": row["kind"],
                "slot_count": row["slot_count"],
            },
        )

    for line, row in _rows(base / "vtable-slots.csv"):
        yield Fact(
            "vtable-slot",
            f"vtable-id:{row['program']}:{row['vtable_id']}",
            f"function:{row['program']}:{row['target']}",
            _confidence_tier(row["confidence"]),
            row["program"],
            f"evidence/reviewed/wiz8/vtable-slots.csv:{line}",
            {"slot": row["slot_index"], "name": row["slot_name"] or None},
        )


def _boundary_facts(repo: Path) -> Iterable[Fact]:
    path = repo / "config" / "reccmp" / "wiz8-gameplay-boundaries.csv"
    for line, row in _rows(path):
        yield Fact(
            "matches-source-body",
            f"function:wiz8:{row['address']}",
            f"symbol:{row['symbol']}",
            _confidence_tier(row["confidence"]),
            "wiz8",
            f"config/reccmp/wiz8-gameplay-boundaries.csv:{line}",
            {
                "size": row["size"],
                "digest": row["relocation_masked_sha256"] or None,
                "owner": row["owner"],
            },
        )


def _snapshot_facts(repo: Path) -> Iterable[Fact]:
    snapshots = repo / "evidence" / "snapshots"

    for line, row in _rows(snapshots / "polymorphism" / "vtables.csv"):
        yield Fact(
            "vtable-observed",
            f"vtable:{row['program']}:{row['address']}",
            None,
            "observation",
            row["program"],
            f"evidence/snapshots/polymorphism/vtables.csv:{line}",
            {
                "kind": row["kind"],
                "slot_count": row["slot_count"],
                "import_slots": row["import_slots"],
                "pure_virtual_slots": row["pure_virtual_slots"],
                "allocation_sizes": row["allocation_sizes"] or None,
            },
        )

    for line, row in _rows(snapshots / "polymorphism" / "vptr-writes.csv"):
        yield Fact(
            "installs-vptr",
            f"function:{row['program']}:{row['function_start'] or row['site']}",
            f"vtable:{row['program']}:{row['vtable']}",
            "observation",
            row["program"],
            f"evidence/snapshots/polymorphism/vptr-writes.csv:{line}",
            {
                "site": row["site"],
                "object_offset": row["object_offset"],
                "allocation_size": row.get("allocation_size") or None,
            },
        )

    for line, row in _rows(snapshots / "polymorphism" / "slots.csv"):
        yield Fact(
            "vtable-slot-observed",
            f"vtable:{row['program']}:{row['vtable']}",
            f"function:{row['program']}:{row['target']}" if row["target"] else None,
            "observation",
            row["program"],
            f"evidence/snapshots/polymorphism/slots.csv:{line}",
            {
                "slot": row["slot_index"],
                "kind": row["kind"],
                "import_name": row["import_name"] or None,
            },
        )

    for line, row in _rows(snapshots / "functions" / "calls.csv"):
        yield Fact(
            "calls",
            f"function:{row['program']}:{row['caller']}",
            f"function:{row['program']}:{row['callee']}",
            "observation",
            row["program"],
            f"evidence/snapshots/functions/calls.csv:{line}",
            {"sites": row["call_sites"]},
        )

    for line, row in _rows(snapshots / "call-sites" / "assertions.csv"):
        if not row["source_path"]:
            continue
        unit = row["source_path"].split("Wizardry 8\\", 1)[-1]
        yield Fact(
            "asserts-in-unit",
            f"function:{row['program']}:{row['function_start'] or row['call_site']}",
            f"unit:{unit}",
            "observation",
            row["program"],
            f"evidence/snapshots/call-sites/assertions.csv:{line}",
            {"line": row["line"], "expression": row["expression"][:200]},
        )

    for line, row in _rows(snapshots / "eh-metadata" / "functions.csv"):
        yield Fact(
            "eh-frame",
            f"function:{row['program']}:{row['frame_setup']}",
            None,
            "observation",
            row["program"],
            f"evidence/snapshots/eh-metadata/functions.csv:{line}",
            {"funcinfo": row["funcinfo"], "unwind": row["unwind_signature"][:200]},
        )

    for line, row in _rows(snapshots / "surrender-abi" / "exports.csv"):
        if not row["class_name"]:
            continue
        yield Fact(
            "imported-member",
            f"imported-type:{row['class_name']}",
            f"import:{row['decorated_name']}",
            "observation",
            row["program"],
            f"evidence/snapshots/surrender-abi/exports.csv:{line}",
            {
                "kind": row["kind"],
                "virtuality": row["virtuality"],
                "signature": row["demangled_signature"][:200],
            },
        )

    for line, row in _rows(snapshots / "surrender-abi" / "vftable-slots.csv"):
        yield Fact(
            "imported-vtable-slot",
            f"imported-vtable:{row['program']}:{row['table']}",
            f"import:{row['target_name']}" if row["target_name"] else None,
            "observation",
            row["program"],
            f"evidence/snapshots/surrender-abi/vftable-slots.csv:{line}",
            {"slot": row["slot"], "class": row["class_name"], "resolution": row["resolution"]},
        )


def _derived_candidate_facts(
    connection: sqlite3.Connection, program: str = _CANONICAL
) -> Iterable[tuple[Fact, list[int]]]:
    """Class candidates derived inside the store, with parent edges.

    The derivation mirrors what the candidate report computes - one candidate
    per constructor-written vftable - but here each candidate fact carries
    edges to the vtable observation and every write observation it rests on,
    so `why candidate-class:...` answers with its actual inputs.
    """

    vtables = {
        row[1]: row[0]
        for row in connection.execute(
            "SELECT id, subject FROM fact WHERE kind = 'vtable-observed' AND program = ?",
            (program,),
        )
    }
    writes: dict[str, list[tuple[int, str, str]]] = {}
    for fact_id, vtable, attributes in connection.execute(
        "SELECT id, object, attributes FROM fact WHERE kind = 'installs-vptr' AND program = ?",
        (program,),
    ):
        writes.setdefault(vtable, []).append((fact_id, vtable, attributes))

    for vtable_subject, vtable_fact in sorted(vtables.items()):
        vtable_writes = writes.get(vtable_subject, [])
        primary = [
            item
            for item in vtable_writes
            if json.loads(item[2]).get("object_offset") == "0x0"
        ]
        if not primary:
            continue
        address = vtable_subject.rsplit(":", 1)[-1]
        yield (
            Fact(
                "candidate-class",
                f"candidate:{program}:{address}",
                vtable_subject,
                "candidate",
                program,
                f"derived:candidate-class:{address}",
                {"writer_count": len(primary)},
            ),
            [vtable_fact] + [item[0] for item in primary],
        )


def build_store(settings: Settings, destination: Path | None = None) -> dict[str, Any]:
    """Rebuild the store from the canonical inputs; deterministic and disposable."""

    path = destination or store_path(settings)
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists():
        path.unlink()
    connection = sqlite3.connect(path)
    try:
        connection.executescript(_SCHEMA)
        count = 0
        for fact in (
            *_reviewed_facts(settings.repo_dir),
            *_boundary_facts(settings.repo_dir),
            *_snapshot_facts(settings.repo_dir),
        ):
            connection.execute(
                "INSERT INTO fact (kind, subject, object, tier, program, source, attributes)"
                " VALUES (?, ?, ?, ?, ?, ?, ?)",
                (
                    fact.kind,
                    fact.subject,
                    fact.object,
                    fact.tier,
                    fact.program,
                    fact.source,
                    json.dumps(fact.attributes, sort_keys=True),
                ),
            )
            count += 1
        derived = 0
        for fact, parents in _derived_candidate_facts(connection):
            cursor = connection.execute(
                "INSERT INTO fact (kind, subject, object, tier, program, source, attributes)"
                " VALUES (?, ?, ?, ?, ?, ?, ?)",
                (
                    fact.kind,
                    fact.subject,
                    fact.object,
                    fact.tier,
                    fact.program,
                    fact.source,
                    json.dumps(fact.attributes, sort_keys=True),
                ),
            )
            for parent in parents:
                connection.execute(
                    "INSERT INTO derivation (fact, parent) VALUES (?, ?)",
                    (cursor.lastrowid, parent),
                )
            derived += 1
        connection.commit()
        kinds = dict(
            connection.execute("SELECT kind, COUNT(*) FROM fact GROUP BY kind ORDER BY kind")
        )
        return {
            "store": str(path),
            "facts": count + derived,
            "derived": derived,
            "kinds": kinds,
        }
    finally:
        connection.close()


def content_digest(path: Path) -> str:
    """A canonical hash of the store's content, independent of page layout."""

    import hashlib

    connection = sqlite3.connect(path)
    try:
        digest = hashlib.sha256()
        for row in connection.execute(
            "SELECT kind, subject, object, tier, program, source, attributes"
            " FROM fact ORDER BY kind, subject, object, source"
        ):
            digest.update(repr(row).encode())
        for row in connection.execute(
            "SELECT f.source, p.source FROM derivation d"
            " JOIN fact f ON f.id = d.fact JOIN fact p ON p.id = d.parent"
            " ORDER BY f.source, p.source"
        ):
            digest.update(repr(row).encode())
        return digest.hexdigest()
    finally:
        connection.close()


def why(settings: Settings, subject: str, depth: int = 5, store: Path | None = None) -> dict[str, Any]:
    """The facts about a subject, each with its provenance chain."""

    connection = sqlite3.connect(store or store_path(settings))
    try:

        def parents_of(fact_id: int, remaining: int) -> list[dict[str, Any]]:
            if remaining <= 0:
                return []
            entries = []
            for row in connection.execute(
                "SELECT f.id, f.kind, f.subject, f.object, f.tier, f.source, f.attributes"
                " FROM derivation d JOIN fact f ON f.id = d.parent WHERE d.fact = ?",
                (fact_id,),
            ):
                entries.append(_entry(row, parents_of(row[0], remaining - 1)))
            return entries

        def _entry(row: Any, parents: list[dict[str, Any]]) -> dict[str, Any]:
            return {
                "kind": row[1],
                "subject": row[2],
                "object": row[3],
                "tier": row[4],
                "source": row[5],
                "attributes": json.loads(row[6]),
                **({"derived_from": parents} if parents else {}),
            }

        facts = [
            _entry(row, parents_of(row[0], depth))
            for row in connection.execute(
                "SELECT id, kind, subject, object, tier, source, attributes FROM fact"
                " WHERE subject = ? OR object = ? ORDER BY kind, source",
                (subject, subject),
            )
        ]
        if not facts:
            raise ValueError(f"no facts about {subject}")
        return {"subject": subject, "facts": facts}
    finally:
        connection.close()
