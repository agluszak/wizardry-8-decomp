"""Atomic ProgramDB candidate-fact storage without a JVM."""

from __future__ import annotations

import json
from types import SimpleNamespace

from wiz8decomp.ghidra.candidate_facts import (
    CANDIDATE_FACTS,
    facts,
    remove_fact,
    stamp,
    supersede_fact,
    upsert_fact,
)


class PropertyMap:
    def __init__(self) -> None:
        self.data = {}

    def hasProperty(self, address):
        return address in self.data

    def get(self, address):
        return self.data[address]

    def add(self, address, value):
        self.data[address] = value

    def remove(self, address):
        self.data.pop(address, None)


class Manager:
    def __init__(self) -> None:
        self.maps = {}

    def getStringPropertyMap(self, name):
        return self.maps.get(name)

    def createStringPropertyMap(self, name):
        return self.maps.setdefault(name, PropertyMap())


def program():
    manager = Manager()
    return SimpleNamespace(getUsrPropertyManager=lambda: manager), manager


def test_same_anchor_facts_keep_their_own_constraints() -> None:
    target, manager = program()
    stamp(
        target,
        "0044bec0",
        hypothesis="owned-types",
        fact_id="TypeA",
        depends_on=["pcode:0044bf16"],
        constraints={"complete_destructor": "004b6ed0"},
        type_variable="field38",
    )
    stamp(
        target,
        "0044bec0",
        hypothesis="owned-types",
        fact_id="TypeB",
        depends_on=["pcode:0044bf20"],
        constraints={"complete_destructor": "004c0000"},
        type_variable="field40",
    )

    decoded = json.loads(manager.maps[CANDIDATE_FACTS].data["0044bec0"])
    assert decoded["TypeA"]["payload"]["complete_destructor"] == "004b6ed0"
    assert decoded["TypeB"]["payload"]["complete_destructor"] == "004c0000"
    assert set(manager.maps) == {"wiz8.layer", CANDIDATE_FACTS}


def test_target_set_payload_is_replaced_not_appended() -> None:
    target, _manager = program()
    common = {
        "program": target,
        "address": "00401000",
        "fact_id": "virtual-target-set:00401000",
        "hypothesis": "dispatch",
        "kind": "target-set",
        "depends_on": ["vtable:GrCycle.primary"],
    }
    upsert_fact(**common, payload={"vtable_id": "GrCycle.primary", "targets": ["a", "b"]})
    upsert_fact(**common, payload={"vtable_id": "GrCycle.primary", "targets": ["b"]})

    assert facts(target, "00401000")["virtual-target-set:00401000"]["payload"][
        "targets"
    ] == ["b"]


def test_incompatible_constraint_becomes_a_contradiction() -> None:
    target, _manager = program()
    common = {
        "program": target,
        "address": "00401000",
        "fact_id": "AnonymousType",
        "hypothesis": "owned-types",
        "kind": "type-variable",
        "depends_on": [],
    }
    upsert_fact(**common, payload={"complete_destructor": "004b6ed0"})
    upsert_fact(**common, payload={"complete_destructor": "004c0000"})

    record = facts(target, "00401000")["AnonymousType"]
    assert record["status"] == "contradicted"
    assert record["payload"]["complete_destructor"] == "004b6ed0"
    assert record["contradictions"]


def test_fact_lifecycle_is_owned_by_fact_id() -> None:
    target, manager = program()
    upsert_fact(
        target,
        "00401000",
        fact_id="old",
        hypothesis="h",
        kind="constraint",
        depends_on=[],
        payload={"value": 1},
    )
    assert supersede_fact(target, "00401000", "old", superseded_by="new")
    assert facts(target, "00401000")["old"]["status"] == "superseded"
    assert remove_fact(target, "00401000", "old")
    assert "00401000" not in manager.maps[CANDIDATE_FACTS].data
