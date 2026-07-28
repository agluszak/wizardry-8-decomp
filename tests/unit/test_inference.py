"""Pure plan and graph contracts for overlay inference."""

from __future__ import annotations

import json
from types import SimpleNamespace

import pytest
from wiz8decomp.ghidra.dependency_graph import _type_node
from wiz8decomp.ghidra.inference import _candidate_type_knowledge, load_plan


def test_a_hypothesis_plan_has_explicit_scope_and_bounded_defaults(tmp_path) -> None:
    path = tmp_path / "plan.json"
    path.write_text(
        json.dumps(
            {
                "hypothesis": "prop-closure",
                "type_variables": [{"function": "0044bec0", "root": "this"}],
            }
        ),
        encoding="utf-8",
    )

    plan = load_plan(tmp_path, str(path))

    assert plan["hypothesis"] == "prop-closure"
    assert plan["max_iterations"] == 8
    assert plan["screen_dispatch"] is False
    assert plan["vtables"] == []


def test_analyze_never_guesses_a_plan_from_a_hypothesis_name(tmp_path) -> None:
    with pytest.raises(ValueError, match="hypothesis plan does not exist"):
        load_plan(tmp_path, "plausible-name")


def test_dependency_types_exclude_primitives_but_keep_owned_shapes() -> None:
    primitive = SimpleNamespace(getDisplayName=lambda: "undefined4")
    receiver = SimpleNamespace(getDisplayName=lambda: "GrCycle *")

    assert _type_node(primitive) is None
    assert _type_node(receiver) == "type:GrCycle *"


def test_a_persisted_candidate_destructor_becomes_next_iteration_knowledge() -> None:
    class Iterator:
        def __init__(self, items):
            self.items = iter(items)
            self.next_item = None

        def hasNext(self):
            try:
                self.next_item = next(self.items)
            except StopIteration:
                return False
            return True

        def next(self):
            return self.next_item

    class PropertyMap:
        def __init__(self, data):
            self.data = data

        def getPropertyIterator(self):
            return Iterator(self.data)

        def hasProperty(self, address):
            return address in self.data

        def get(self, address):
            return self.data[address]

    maps = {
        "wiz8.fact-id": PropertyMap({"004b6ed0": '["CandidateType_004b6ed0"]'}),
        "wiz8.constraints": PropertyMap({"004b6ed0": '[{"complete_destructor":"004b6ed0"}]'}),
    }
    manager = SimpleNamespace(getStringPropertyMap=maps.get)
    program = SimpleNamespace(getUsrPropertyManager=lambda: manager)

    assert _candidate_type_knowledge(program) == [
        {
            "type": "CandidateType_004b6ed0",
            "tier": "candidate",
            "polymorphic": False,
            "slot0": None,
            "destructors": {"004b6ed0"},
        }
    ]
