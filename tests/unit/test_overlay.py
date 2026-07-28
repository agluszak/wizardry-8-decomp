"""Overlay naming and isolation - the parts that need no JVM."""

from __future__ import annotations

from pathlib import Path
from types import SimpleNamespace

import pytest
from wiz8decomp.ghidra.overlay import (
    _reviewed_target_receivers,
    _reviewed_vtable,
    _scratch_dir,
    _slug,
    overlay_identity,
)

REPOSITORY = Path(__file__).resolve().parents[2]


def test_a_hypothesis_name_becomes_a_stable_directory_key() -> None:
    assert _slug("Typed Monster vtable") == "typed-monster-vtable"
    assert _slug("widget/owner=Controls") == "widget-owner-controls"
    with pytest.raises(ValueError, match="no usable characters"):
        _slug("///")


def test_the_scratch_clone_never_lands_inside_the_reviewed_project() -> None:
    # Isolation is a path property: the clone is a sibling of the
    # content-addressed materialization, so discarding it cannot touch the
    # reviewed baseline.
    effective = SimpleNamespace(project_dir=Path("/w/projects/75ff74a1bc16"))
    scratch = _scratch_dir(effective, "typed-monster")

    assert scratch == Path("/w/projects/scratch-typed-monster")
    assert effective.project_dir not in scratch.parents
    assert scratch != effective.project_dir


def test_overlay_identity_changes_with_baseline_plan_and_analyzer() -> None:
    common = {
        "program": "wiz8--gog-base--wiz8--18a74ff61c65",
        "baseline_materialization": "baseline-a",
        "plan_sha256": "plan-a",
        "hypothesis": "owned types",
        "analyzer_version": "analyzer-a",
    }
    first = overlay_identity(**common)

    assert first.startswith("owned-types-")
    assert overlay_identity(**{**common, "baseline_materialization": "baseline-b"}) != first
    assert overlay_identity(**{**common, "plan_sha256": "plan-b"}) != first
    assert overlay_identity(**{**common, "analyzer_version": "analyzer-b"}) != first


def test_the_slot_list_comes_from_the_reviewed_ledger_in_order() -> None:
    vtable, slots = _reviewed_vtable(REPOSITORY, "Monster")

    assert vtable["vtable_id"] == "Monster.primary"
    assert [row["slot_index"] for row in slots] == [str(i) for i in range(len(slots))]
    assert slots[0]["target"] == "004beba0"


def test_a_class_with_no_reviewed_primary_vtable_is_refused() -> None:
    with pytest.raises(ValueError, match="no reviewed primary vtable"):
        _reviewed_vtable(REPOSITORY, "W8ControlsRect")


def test_shared_slot_bodies_keep_all_reviewed_receiver_identities() -> None:
    receivers = _reviewed_target_receivers(REPOSITORY)

    assert receivers["004a7140"] == {
        "GrCycle",
        "GrCycle.subobject_0x18",
        "Monster.subobject_0x18",
    }


def test_the_dependency_cone_names_why_each_function_is_in_it() -> None:
    from wiz8decomp.ghidra.dependency_graph import DependencyGraph

    graph = DependencyGraph()
    graph.add("005ece78", "004a5f00", "vtable-slot")
    graph.add("005ece78", "004a5e50", "vptr-write")
    graph.add("004a5f00", "00401234", "computed-call")

    cone = graph.cone(["005ece78"])

    assert cone["vtable-slot"] == ["004a5f00"]
    assert cone["vptr-write"] == ["004a5e50"]
    assert "00401234" in cone["computed-call"]


def test_dependency_closure_does_not_walk_back_through_a_common_global() -> None:
    from wiz8decomp.ghidra.dependency_graph import DependencyGraph

    graph = DependencyGraph()
    graph.add("00600000", "00401000", "data-reference")
    graph.add("00600000", "00402000", "data-reference")

    assert graph.cone(["00401000"]) == {}
    assert set(graph.cone(["00600000"])["data-reference"]) == {"00401000", "00402000"}


def test_dependency_closure_reports_an_unconsumed_frontier() -> None:
    from wiz8decomp.ghidra.dependency_graph import DependencyGraph

    graph = DependencyGraph()
    graph.add("type:a", "type:b", "one")
    graph.add("type:b", "type:c", "two")

    closure = graph.closure(["type:a"], limit=2)

    assert closure["scope_complete"] is False
    assert closure["truncated_frontier"] == ["type:c"]


def test_a_class_outside_the_reviewed_model_has_an_empty_cone() -> None:
    from wiz8decomp.ghidra.dependency_graph import DependencyGraph

    cone = DependencyGraph().cone(["00500000"])

    assert cone == {}
