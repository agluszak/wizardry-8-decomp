"""Overlay naming and isolation - the parts that need no JVM."""

from __future__ import annotations

from pathlib import Path
from types import SimpleNamespace

import pytest
from wiz8decomp.ghidra.overlay import _reviewed_vtable, _scratch_dir, _slug

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


def test_the_slot_list_comes_from_the_reviewed_ledger_in_order() -> None:
    vtable, slots = _reviewed_vtable(REPOSITORY, "Monster")

    assert vtable["vtable_id"] == "Monster.primary"
    assert [row["slot_index"] for row in slots] == [str(i) for i in range(len(slots))]
    assert slots[0]["target"] == "004beba0"


def test_a_class_with_no_reviewed_primary_vtable_is_refused() -> None:
    with pytest.raises(ValueError, match="no reviewed primary vtable"):
        _reviewed_vtable(REPOSITORY, "Controls")


def test_the_dependency_cone_names_why_each_function_is_in_it() -> None:
    from wiz8decomp.ghidra.dependency_graph import DependencyGraph

    graph = DependencyGraph()
    graph.add("005ece78", "004a5f00", "vtable-slot")
    graph.add("004a5e50", "005ece78", "vptr-write")
    graph.add("00401234", "004a5f00", "computed-call")

    cone = graph.cone(["005ece78"])

    assert cone["vtable-slot"] == ["004a5f00"]
    assert cone["vptr-write"] == ["004a5e50"]
    assert "00401234" in cone["computed-call"]


def test_a_class_outside_the_reviewed_model_has_an_empty_cone() -> None:
    from wiz8decomp.ghidra.dependency_graph import DependencyGraph

    cone = DependencyGraph().cone(["00500000"])

    assert cone == {}
