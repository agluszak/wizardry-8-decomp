"""Pure plan and graph contracts for overlay inference."""

from __future__ import annotations

import json
from types import SimpleNamespace

import pytest
from wiz8decomp.ghidra.dependency_graph import _type_node
from wiz8decomp.ghidra.inference import load_plan


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
