"""Candidate facts persisted inside disposable ProgramDB overlays."""

from __future__ import annotations

import json
from typing import Any

LAYER = "wiz8.layer"
CANDIDATE_LAYER = "wiz8.candidate-layer"
HYPOTHESIS = "wiz8.hypothesis"
FACT_ID = "wiz8.fact-id"
DEPENDS_ON = "wiz8.depends-on"
CONSTRAINTS = "wiz8.constraints"
TYPE_VARIABLE = "wiz8.type-variable"
TARGET_SET = "wiz8.target-set"
UNIFIED_WITH = "wiz8.unified-with"


def _map(program: Any, name: str) -> Any:
    manager = program.getUsrPropertyManager()
    return manager.getStringPropertyMap(name) or manager.createStringPropertyMap(name)


def values(program: Any, address: Any, name: str) -> list[Any]:
    property_map = program.getUsrPropertyManager().getStringPropertyMap(name)
    if property_map is None or not property_map.hasProperty(address):
        return []
    raw = str(property_map.get(address) or "")
    try:
        decoded = json.loads(raw)
        return decoded if isinstance(decoded, list) else [decoded]
    except json.JSONDecodeError:
        return [raw]


def append(program: Any, address: Any, name: str, value: Any) -> bool:
    """Append a stable JSON value without duplicating an existing fact."""

    existing = values(program, address, name)
    canonical = json.dumps(value, sort_keys=True, separators=(",", ":"))
    if any(
        json.dumps(item, sort_keys=True, separators=(",", ":")) == canonical for item in existing
    ):
        return False
    existing.append(value)
    _map(program, name).add(address, json.dumps(existing, sort_keys=True, separators=(",", ":")))
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
    """Write one reviewable candidate record at its evidence anchor."""

    changed = False
    layer = _map(program, LAYER)
    if not layer.hasProperty(address):
        layer.add(address, "candidate")
        changed = True
    elif str(layer.get(address)) != "candidate":
        changed |= append(program, address, CANDIDATE_LAYER, "candidate")
    changed |= append(program, address, HYPOTHESIS, hypothesis)
    changed |= append(program, address, FACT_ID, fact_id)
    changed |= append(program, address, DEPENDS_ON, depends_on)
    if constraints is not None:
        changed |= append(program, address, CONSTRAINTS, constraints)
    if type_variable is not None:
        changed |= append(program, address, TYPE_VARIABLE, type_variable)
    if target_set is not None:
        changed |= append(program, address, TARGET_SET, target_set)
    if unified_with is not None:
        changed |= append(program, address, UNIFIED_WITH, unified_with)
    return changed
