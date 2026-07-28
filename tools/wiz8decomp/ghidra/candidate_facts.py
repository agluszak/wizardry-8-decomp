"""Atomic candidate facts persisted inside disposable ProgramDB overlays.

One address property contains complete records keyed by fact ID. Keeping the
identity, dependencies and payload together prevents cross-pairing unrelated
facts that happen to share an anchor.
"""

from __future__ import annotations

import json
from collections.abc import Iterator
from typing import Any

LAYER = "wiz8.layer"
CANDIDATE_FACTS = "wiz8.candidate-facts"


def _map(program: Any) -> Any:
    manager = program.getUsrPropertyManager()
    return manager.getStringPropertyMap(CANDIDATE_FACTS) or manager.createStringPropertyMap(
        CANDIDATE_FACTS
    )


def facts(program: Any, address: Any) -> dict[str, dict[str, Any]]:
    """Return the complete candidate-fact object stored at ``address``."""

    property_map = program.getUsrPropertyManager().getStringPropertyMap(CANDIDATE_FACTS)
    if property_map is None or not property_map.hasProperty(address):
        return {}
    try:
        decoded = json.loads(str(property_map.get(address) or "{}"))
    except json.JSONDecodeError:
        return {}
    if not isinstance(decoded, dict):
        return {}
    return {str(fact_id): record for fact_id, record in decoded.items() if isinstance(record, dict)}


def iter_facts(program: Any) -> Iterator[tuple[Any, str, dict[str, Any]]]:
    """Yield ``(anchor, fact_id, record)`` without reconstructing joins."""

    property_map = program.getUsrPropertyManager().getStringPropertyMap(CANDIDATE_FACTS)
    if property_map is None:
        return
    iterator = property_map.getPropertyIterator()
    while iterator.hasNext():
        address = iterator.next()
        for fact_id, record in facts(program, address).items():
            yield address, fact_id, record


def get_fact(program: Any, address: Any, fact_id: str) -> dict[str, Any] | None:
    return facts(program, address).get(fact_id)


def _write(program: Any, address: Any, records: dict[str, dict[str, Any]]) -> None:
    property_map = _map(program)
    if records:
        property_map.add(address, json.dumps(records, sort_keys=True, separators=(",", ":")))
    else:
        property_map.remove(address)


def _canonical(value: Any) -> str:
    return json.dumps(value, sort_keys=True, separators=(",", ":"))


def _merge_constraints(previous: Any, incoming: Any, path: str = "payload") -> Any:
    """Merge an evidence refinement, rejecting incompatible established values."""

    if previous is None:
        return incoming
    if incoming is None or _canonical(previous) == _canonical(incoming):
        return previous
    if isinstance(previous, dict) and isinstance(incoming, dict):
        merged = dict(previous)
        for key, value in incoming.items():
            merged[key] = _merge_constraints(previous.get(key), value, f"{path}.{key}")
        return merged
    if isinstance(previous, list) and isinstance(incoming, list):
        merged = list(previous)
        seen = {_canonical(item) for item in merged}
        for item in incoming:
            canonical = _canonical(item)
            if canonical not in seen:
                merged.append(item)
                seen.add(canonical)
        return merged
    raise ValueError(f"incompatible candidate constraint at {path}: {previous!r} != {incoming!r}")


def record_contradiction(
    program: Any,
    address: Any,
    fact_id: str,
    *,
    reason: str,
    incoming: Any | None = None,
) -> bool:
    """Mark one owned fact contradicted while preserving both sides for review."""

    records = facts(program, address)
    previous = records.get(fact_id)
    if previous is None:
        previous = {
            "hypothesis": "unknown",
            "kind": "contradiction",
            "depends_on": [],
            "payload": {},
            "status": "candidate",
        }
    updated = dict(previous)
    contradictions = list(updated.get("contradictions", []))
    contradiction = {"reason": reason}
    if incoming is not None:
        contradiction["incoming"] = incoming
    if _canonical(contradiction) not in {_canonical(item) for item in contradictions}:
        contradictions.append(contradiction)
    updated["contradictions"] = contradictions
    updated["status"] = "contradicted"
    if _canonical(previous) == _canonical(updated):
        return False
    records[fact_id] = updated
    _write(program, address, records)
    return True


def upsert_fact(
    program: Any,
    address: Any,
    *,
    fact_id: str,
    hypothesis: str,
    kind: str,
    depends_on: list[str],
    payload: dict[str, Any],
    status: str = "candidate",
) -> bool:
    """Insert or update one whole fact.

    Target sets are replaceable owned outputs. Other facts may only accumulate
    compatible constraints; a conflict records a contradiction while retaining
    the last coherent payload.
    """

    records = facts(program, address)
    previous = records.get(fact_id)
    record = {
        "hypothesis": hypothesis,
        "kind": kind,
        "depends_on": sorted(set(depends_on)),
        "payload": payload,
        "status": status,
    }
    if previous is not None and kind != "target-set":
        if previous.get("kind") != kind or previous.get("hypothesis") != hypothesis:
            return record_contradiction(
                program,
                address,
                fact_id,
                reason="fact identity changed kind or hypothesis",
                incoming=record,
            )
        try:
            record["payload"] = _merge_constraints(previous.get("payload", {}), payload)
        except ValueError as error:
            return record_contradiction(
                program, address, fact_id, reason=str(error), incoming=payload
            )
        record["depends_on"] = sorted(
            set(previous.get("depends_on", [])) | set(record["depends_on"])
        )
        if previous.get("status") in {"contradicted", "superseded"}:
            record["status"] = previous["status"]
        if previous.get("contradictions"):
            record["contradictions"] = previous["contradictions"]
    if previous is not None and _canonical(previous) == _canonical(record):
        return False
    records[fact_id] = record
    _write(program, address, records)

    layer = program.getUsrPropertyManager().getStringPropertyMap(LAYER)
    if layer is None:
        layer = program.getUsrPropertyManager().createStringPropertyMap(LAYER)
    if not layer.hasProperty(address):
        layer.add(address, "candidate")
    return True


def supersede_fact(program: Any, address: Any, fact_id: str, *, superseded_by: str) -> bool:
    records = facts(program, address)
    previous = records.get(fact_id)
    if previous is None:
        return False
    updated = dict(previous)
    updated["status"] = "superseded"
    updated["superseded_by"] = superseded_by
    if _canonical(previous) == _canonical(updated):
        return False
    records[fact_id] = updated
    _write(program, address, records)
    return True


def remove_fact(program: Any, address: Any, fact_id: str) -> bool:
    records = facts(program, address)
    if fact_id not in records:
        return False
    del records[fact_id]
    _write(program, address, records)
    return True


def stamp(
    program: Any,
    address: Any,
    *,
    hypothesis: str,
    fact_id: str,
    depends_on: list[str],
    constraints: dict[str, Any] | None = None,
    type_variable: str | None = None,
    target_set: dict[str, Any] | None = None,
    unified_with: str | None = None,
) -> bool:
    """Compatibility-shaped call surface that writes one atomic record."""

    if target_set is not None:
        kind = "target-set"
        payload = dict(target_set)
    else:
        kind = "type-variable" if type_variable is not None else "constraint"
        payload = dict(constraints or {})
        if type_variable is not None:
            payload["type_variable"] = type_variable
        if unified_with is not None:
            payload["unified_with"] = unified_with
    return upsert_fact(
        program,
        address,
        fact_id=fact_id,
        hypothesis=hypothesis,
        kind=kind,
        depends_on=depends_on,
        payload=payload,
    )
